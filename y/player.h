
#pragma once

//A Monte-Carlo Tree Search based player

#include <cmath>
#include <cassert>

#include "../lib/compacttree.h"
#include "../lib/depthstats.h"
#include "../lib/exppair.h"
#include "../lib/log.h"
#include "../lib/time.h"
#include "../lib/thread.h"
#include "../lib/types.h"
#include "../lib/xorshift.h"

#include "board.h"
#include "lbdist.h"
#include "move.h"
#include "policy_bridge.h"
#include "policy_instantwin.h"
#include "policy_lastgoodreply.h"
#include "policy_random.h"


class Player {
public:

	struct Node {
	public:
		ExpPair rave;
		ExpPair exp;
		int16_t know;
		int8_t  outcome;
		uint8_t proofdepth;
		Move    move;
		Move    bestmove; //if outcome is set, then bestmove is the way to get there
		CompactTree<Node>::Children children;
//		int padding;
		//seems to need padding to multiples of 8 bytes or it segfaults?
		//don't forget to update the copy constructor/operator

		Node()                            : know(0), outcome(-3), proofdepth(0)          { }
		Node(const Move & m, char o = -3) : know(0), outcome( o), proofdepth(0), move(m) { }
		Node(const Node & n) { *this = n; }
		Node & operator = (const Node & n){
			if(this != & n){ //don't copy to self
				//don't copy to a node that already has children
				assert(children.empty());

				rave = n.rave;
				exp  = n.exp;
				know = n.know;
				move = n.move;
				bestmove = n.bestmove;
				outcome = n.outcome;
				proofdepth = n.proofdepth;
				//children = n.children; ignore the children, they need to be swap_tree'd in
			}
			return *this;
		}

		void swap_tree(Node & n){
			children.swap(n.children);
		}

		void print() const {
			printf("%s\n", to_s().c_str());
		}
		string to_s() const {
			return "Node: move " + move.to_s() +
					", exp " + to_str(exp.avg(), 2) + "/" + to_str(exp.num()) +
					", rave " + to_str(rave.avg(), 2) + "/" + to_str(rave.num()) +
					", know " + to_str(know) +
					", outcome " + to_str(outcome) + "/" + to_str(proofdepth) +
					", best " + bestmove.to_s() +
					", children " + to_str(children.num());
		}

		unsigned int size() const {
			unsigned int num = children.num();

			if(children.num())
				for(Node * i = children.begin(); i != children.end(); i++)
					num += i->size();

			return num;
		}

		~Node(){
			assert(children.empty());
		}

		unsigned int alloc(unsigned int num, CompactTree<Node> & ct){
			return children.alloc(num, ct);
		}
		unsigned int dealloc(CompactTree<Node> & ct){
			unsigned int num = 0;

			if(children.num())
				for(Node * i = children.begin(); i != children.end(); i++)
					num += i->dealloc(ct);
			num += children.dealloc(ct);

			return num;
		}

		//new way, more standard way of changing over from rave scores to real scores
		float value(float ravefactor, bool knowledge, float fpurgency){
			float val = fpurgency;
			float expnum = exp.num();
			float ravenum = rave.num();

			if(ravefactor <= min_rave){
				if(expnum > 0)
					val = exp.avg();
			}else if(ravenum > 0 || expnum > 0){
				float alpha = ravefactor/(ravefactor + expnum);
//				float alpha = sqrt(ravefactor/(ravefactor + 3.0f*expnum));
//				float alpha = ravenum/(expnum + ravenum + expnum*ravenum*ravefactor);

				val = 0;
				if(ravenum > 0) val += alpha*rave.avg();
				if(expnum  > 0) val += (1.0f-alpha)*exp.avg();
			}

			if(knowledge && know > 0){
				if(expnum <= 1)
					val += 0.01f * know;
				else if(expnum < 1000) //knowledge is only useful with little experience
					val += 0.01f * know / sqrt(expnum);
			}

			return val;
		}
	};

	class PlayerThread {
	protected:
	public:
		mutable XORShift_float unitrand;
		Thread thread;
		Player * player;
	public:
		DepthStats treelen, gamelen;
		double times[4]; //time spent in each of the stages

		PlayerThread() {}
		virtual ~PlayerThread() { }
		virtual void reset() { }
		int join(){ return thread.join(); }
		void run(); //thread runner, calls iterate on each iteration
		virtual void iterate() { } //handles each iteration
	};

	class PlayerUCT : public PlayerThread {
		LastGoodReply last_good_reply;
		RandomPolicy random_policy;
		ProtectBridge protect_bridge;
		InstantWin instant_wins;

		bool use_rave;    //whether to use rave for this simulation
		bool use_explore; //whether to use exploration for this simulation
		LBDists dists;    //holds the distances to the various non-ring wins as a heuristic for the minimum moves needed to win
		MoveList movelist;
		int stage; //which of the four MCTS stages is it on
		Time timestamps[4]; //timestamps for the beginning, before child creation, before rollout, after rollout

	public:
		PlayerUCT(Player * p) : PlayerThread() {
			player = p;
			reset();
			thread(bind(&PlayerUCT::run, this));
		}

		void reset(){
			treelen.reset();
			gamelen.reset();

			use_rave = false;
			use_explore = false;

			for(int a = 0; a < 4; a++)
				times[a] = 0;
		}

	private:
		void iterate();
		void walk_tree(Board & board, Node * node, int depth);
		bool create_children(Board & board, Node * node, int toplay);
		void add_knowledge(Board & board, Node * node, Node * child);
		Node * choose_move(const Node * node, int toplay, int remain) const;
		void update_rave(const Node * node, int toplay);
		bool test_bridge_probe(const Board & board, const Move & move, const Move & test) const;

		int rollout(Board & board, Move move, int depth);
		Move rollout_choose_move(Board & board, const Move & prev);
		Move rollout_pattern(const Board & board, const Move & move);
	};


public:

	static const float min_rave;

	bool  ponder;     //think during opponents time?
	int   numthreads; //number of player threads to run
	u64   maxmem;     //maximum memory for the tree in bytes
	bool  profile;    //count how long is spent in each stage of MCTS
//final move selection
	float msrave;     //rave factor in final move selection, -1 means use number instead of value
	float msexplore;  //the UCT constant in final move selection
//tree traversal
	bool  parentexplore; // whether to multiple exploration by the parents winrate
	float explore;    //greater than one favours exploration, smaller than one favours exploitation
	float ravefactor; //big numbers favour rave scores, small ignore it
	float decrrave;   //decrease rave over time, add this value for each empty position on the board
	bool  knowledge;  //whether to include knowledge
	float userave;    //what probability to use rave
	float useexplore; //what probability to use UCT exploration
	float fpurgency;  //what value to return for a move that hasn't been played yet
	int   rollouts;   //number of rollouts to run after the tree traversal
	float dynwiden;   //dynamic widening, look at first log_dynwiden(experience) number of children, 0 to disable
	float logdynwiden; // = log(dynwiden), cached for performance
//tree building
	bool  shortrave;  //only update rave values on short rollouts
	bool  keeptree;   //reuse the tree from the previous move
	int   minimax;    //solve the minimax tree within the uct tree
	uint  visitexpand;//number of visits before expanding a node
	bool  prunesymmetry; //prune symmetric children from the move list, useful for proving but likely not for playing
	uint  gcsolved;   //garbage collect solved nodes or keep them in the tree, assuming they meet the required amount of work
//knowledge
	int   localreply; //boost for a local reply, ie a move near the previous move
	int   locality;   //boost for playing near previous stones
	int   connect;    //boost for having connections to edges and corners
	int   size;       //boost for large groups
	int   bridge;     //boost replying to a probe at a bridge
	int   dists;      //boost based on minimum number of stones needed to finish a non-ring win
//rollout
	int   weightedrandom; //learn weights based on patterns and outcomes to make better moves
	bool  rolloutpattern; //play the response to a virtual connection threat in rollouts
	int   lastgoodreply;  //use the last-good-reply rollout heuristic
	int   instantwin;     //look for instant wins in rollouts
	int   instwindepth;   //how deep to look for instant wins

	float gammas[4096]; //pattern weights for weighted random

	Board rootboard;
	Node  root;
	uword nodes;
	int   gclimit; //the minimum experience needed to not be garbage collected

	uint64_t runs, maxruns;

	CompactTree<Node> ctmem;

	enum ThreadState {
		Thread_Cancelled,  //threads should exit
		Thread_Wait_Start, //threads are waiting to start
		Thread_Wait_Start_Cancelled, //once done waiting, go to cancelled instead of running
		Thread_Running,    //threads are running
		Thread_GC,         //one thread is running garbage collection, the rest are waiting
		Thread_GC_End,     //once done garbage collecting, go to wait_end instead of back to running
		Thread_Wait_End,   //threads are waiting to end
	};
	volatile ThreadState threadstate;
	vector<PlayerThread *> threads;
	Barrier runbarrier, gcbarrier;

	double time_used;

	Player();
	~Player();

	void timedout();

	string statestring();

	void stop_threads();
	void start_threads();
	void reset_threads();

	void set_ponder(bool p);
	void set_board(const Board & board);

	void move(const Move & m);

	double gamelen();

	Node * genmove(double time, int max_runs, bool flexible);
	vector<Move> get_pv();
	void garbage_collect(Board & board, Node * node); //destroys the board, so pass in a copy

	bool do_backup(Node * node, Node * backup, int toplay);

	void gen_hgf(Board & board, Node * node, unsigned int limit, unsigned int depth, FILE * fd);
	void load_hgf(Board board, Node * node, FILE * fd);

	void create_children_simple(const Board & board, Node * node);
	Node * find_child(Node * node, const Move & move);

protected:
	Node * return_move(Node * node, int toplay) const;
};
