
#pragma once

#include <vector>

#include "alarm.h"
#include "log.h"
#include "thread.h"
#include "time.h"
#include "types.h"

/*
This implements a thread pool for agents to use to be multithreaded.

The Agent subclass must define a subclass of AgentThreadBase named AgentThread which
implements AgentThread::iterate() , which gets called repeatedly. AgentThread can have
whichever thread-local variables it wants and can access the base agent through
the agent pointer.

The Agent subclass must also define 3 methods:
	bool done();
	bool need_gc();
	void start_gc();
for telling the thread pool when it should stop the threads to garbage collect, to
call garbage collection, and when the threads have no more work to do, potentially
returning control to the caller of wait_pause()

The threads run in separate threads, not including the main thread, allowing the main
thread to go do something else, allowing pondering.

The main thread and the worker threads are coordinated with a simple state machine and
two barriers.
*/

enum ThreadState {
	Thread_Cancelled,  //threads should exit
	Thread_Wait_Start, //threads are waiting to start
	Thread_Wait_Start_Cancelled, //once done waiting, go to cancelled instead of running
	Thread_Running,    //threads are running
	Thread_GC,         //one thread is running garbage collection, the rest are waiting
	Thread_GC_End,     //once done garbage collecting, go to wait_end instead of back to running
	Thread_Wait_End,   //threads are waiting to end
};

template<typename AgentType> class AgentThreadBase;

template<typename AgentType>
class AgentThreadPool {
	friend class AgentThreadBase<AgentType>;
	volatile ThreadState thread_state;
	unsigned int num_threads;
	std::vector<typename AgentType::AgentThread *> threads;
	Barrier run_barrier, // coordinates starting and finishing
	        gc_barrier;  // coordinates garbage collection
	AgentType * agent;

public:

	AgentThreadPool(AgentType * a) : thread_state(Thread_Wait_Start), num_threads(0), agent(a) {
	}
	~AgentThreadPool(){
		pause();
		set_num_threads(0); //shut down the theads properly
	}

	void timed_out() {
		CAS(thread_state, Thread_Running, Thread_Wait_End);
		CAS(thread_state, Thread_GC, Thread_GC_End);
		agent->timedout();
	}

	int size() const {
		return num_threads;
	}

	typename std::vector<typename AgentType::AgentThread *>::const_iterator begin() const { return threads.begin(); }
	typename std::vector<typename AgentType::AgentThread *>::const_iterator end()   const { return threads.end(); }

	const typename AgentType::AgentThread & operator [] (int i) const {
		return *(threads[i]);
	}

	string state_string() const {
		switch(thread_state){
		case Thread_Cancelled:  return "Thread_Wait_Cancelled";
		case Thread_Wait_Start: return "Thread_Wait_Start";
		case Thread_Wait_Start_Cancelled: return "Thread_Wait_Start_Cancelled";
		case Thread_Running:    return "Thread_Running";
		case Thread_GC:         return "Thread_GC";
		case Thread_GC_End:     return "Thread_GC_End";
		case Thread_Wait_End:   return "Thread_Wait_End";
		}
		return "Thread_State_Unknown!!!";
	}

	void pause() { // pause the threads so they're waiting to be resumed
		if(thread_state != Thread_Wait_Start){
			timed_out();
			run_barrier.wait();
			CAS(thread_state, Thread_Wait_End, Thread_Wait_Start);
			assert(thread_state == Thread_Wait_Start);
		}
	}

	void resume() { // let the threads run
		assert(thread_state == Thread_Wait_Start);
		run_barrier.wait();
		CAS(thread_state, Thread_Wait_Start, Thread_Running);
	}

	void wait_pause(double time) { // wait until they run out of time or otherwise finish
		Alarm timer;
		if(time > 0)
			timer(time, std::bind(&AgentThreadPool::timed_out, this));

		//wait for the timer to stop them or for them to hit the other stop conditions
		run_barrier.wait();
		CAS(thread_state, Thread_Wait_End, Thread_Wait_Start);
		assert(thread_state == Thread_Wait_Start);
	}

	void set_num_threads(unsigned int new_num_threads) { // destroys the existing threads and creates the specified number of new ones
		//start and end with thread_state = Thread_Wait_Start
		assert(thread_state == Thread_Wait_Start);

		//wait for them to all get to the barrier
		assert(CAS(thread_state, Thread_Wait_Start, Thread_Wait_Start_Cancelled));
		run_barrier.wait();

		//make sure they exited cleanly
		for(unsigned int i = 0; i < threads.size(); i++){
			threads[i]->join();
			delete threads[i];
		}

		threads.clear();

		thread_state = Thread_Wait_Start;

		num_threads = new_num_threads;

		run_barrier.reset(num_threads + 1);
		gc_barrier.reset(num_threads);

		//start new threads
		for(unsigned int i = 0; i < num_threads; i++)
			threads.push_back(new typename AgentType::AgentThread(this, agent));

		assert(thread_state == Thread_Wait_Start);
	}

	void reset() { // call the reset method on each thread
		//start and end with thread_state = Thread_Wait_Start
		assert(thread_state == Thread_Wait_Start);

		for(auto & t : threads)
			t->reset();

		assert(thread_state == Thread_Wait_Start);
	}
};

template<typename AgentType>
class AgentThreadBase {
private:
	Thread thread;
	AgentThreadPool<AgentType> * pool;

protected:
	AgentType * agent;

public:

	AgentThreadBase(AgentThreadPool<AgentType> * p, AgentType * a) : pool(p), agent(a) {
		reset();
		thread(bind(&AgentThreadBase::run, this));
	}
	virtual ~AgentThreadBase() { }

	virtual void init() { }  // for setting up the subclass's variables
	virtual void reset() { } // for getting ready for the next search

	int join(){ return thread.join(); }

private:

	void run(){
		while(true){
			switch(pool->thread_state){
			case Thread_Cancelled:  //threads should exit
				return;

			case Thread_Wait_Start: //threads are waiting to start
			case Thread_Wait_Start_Cancelled:
				pool->run_barrier.wait();
				CAS(pool->thread_state, Thread_Wait_Start, Thread_Running);
				CAS(pool->thread_state, Thread_Wait_Start_Cancelled, Thread_Cancelled);
				break;

			case Thread_Wait_End:   //threads are waiting to end
				pool->run_barrier.wait();
				CAS(pool->thread_state, Thread_Wait_End, Thread_Wait_Start);
				break;

			case Thread_Running:    //threads are running
				if(agent->done()){ //solved or finished runs
					CAS(pool->thread_state, Thread_Running, Thread_Wait_End);
					break;
				}
				if(agent->need_gc()){ //out of memory, start garbage collection
					CAS(pool->thread_state, Thread_Running, Thread_GC);
					break;
				}
				iterate();
				break;

			case Thread_GC:         //one thread is running garbage collection, the rest are waiting
			case Thread_GC_End:     //once done garbage collecting, go to wait_end instead of back to running
				if(pool->gc_barrier.wait()){
					agent->start_gc();
					CAS(pool->thread_state, Thread_GC,     Thread_Running);
					CAS(pool->thread_state, Thread_GC_End, Thread_Wait_End);
				}
				pool->gc_barrier.wait();
				break;
			}
		}
	}

	virtual void iterate() = 0; //handles each iteration
};
