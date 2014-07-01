
#pragma once

#include <functional>
#include <unistd.h>

#include "thread.h"

class Timer {
	Thread thread;
	bool destruct;
	std::function<void()> callback;
	double timeout;

	void waiter(){
		sleep((int)timeout);
		usleep((int)((timeout - (int)timeout)*1000000));
		callback();
	}

	void nullcallback(){ }

public:
	Timer() {
		timeout = 0;
		destruct = false;
		callback = std::bind(&Timer::nullcallback, this);
	}

	Timer(double time, std::function<void()> fn){
		destruct = false;
		set(time, fn);
	}

	void set(double time, std::function<void()> fn){
		cancel();

		timeout = time;
		callback = fn;
		if(time < 0.001){ //too small, just call it directly
			destruct = false;
			fn();
		}else{
			destruct = true;
			thread(std::bind(&Timer::waiter, this));
		}
	}

	void cancel(){
		if(destruct){
			destruct = false;
			callback = std::bind(&Timer::nullcallback, this);
			thread.cancel();
			thread.join();
		}
	}

	~Timer(){
		cancel();
	}
};
