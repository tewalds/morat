
#pragma once

//A Monte-Carlo Tree Search based player

#include <cmath>
#include <cassert>

#include "../lib/agentpool.h"
#include "../lib/compacttree.h"
#include "../lib/depthstats.h"
#include "../lib/exppair.h"
#include "../lib/log.h"
#include "../lib/thread.h"
#include "../lib/time.h"
#include "../lib/types.h"
#include "../lib/xorshift.h"

#include "agent.h"
#include "board.h"
#include "move.h"

class AgentMCTS : public Agent{
public:

	struct Node {
	public:
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
					", know " + to_str(know) +
					", outcome " + to_str((int)outcome) + "/" + to_str((int)proofdepth) +
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

		float value(bool knowledge, float fpurgency){
			float val = fpurgency;
			float expnum = exp.num();

			if(expnum > 0)
				val = exp.avg();

			if(knowledge && know != 0){
				if(expnum <= 1)
					val += 0.01f * know;
				else if(expnum < 1000) //knowledge is only useful with little experience
					val += 0.01f * know / sqrt(expnum);
			}

			return val;
		}
	};

	struct MoveList { //intended to be used to track moves for use in rave or similar

		ExpPair  exp[2];       //aggregated outcomes overall

		MoveList() { }

		void addtree(const Move & move, char turn){
		}
		void addrollout(const Move & move, char turn){
		}
		void reset(Board * b){
			exp[0].clear();
			exp[1].clear();
		}
		void finishrollout(int won){
			exp[0].addloss();
			exp[1].addloss();
			if(won == 0){
				exp[0].addtie();
				exp[1].addtie();
			}else{
				exp[won-1].addwin();
			}
		}
		void subvlosses(int n){
			exp[0].addlosses(-n);
			exp[1].addlosses(-n);
		}
		const ExpPair & getexp(int turn) const {
			return exp[turn-1];
		}
	};

	class AgentThread : public AgentThreadBase<AgentMCTS> {
		mutable XORShift_uint64 rand64;
		mutable XORShift_float unitrand;
		bool use_explore; //whether to use exploration for this simulation
		MoveList movelist;
		int stage; //which of the four MCTS stages is it on

	public:
		DepthStats treelen, gamelen;
		double times[4]; //time spent in each of the stages
		Time timestamps[4]; //timestamps for the beginning, before child creation, before rollout, after rollout

		AgentThread(AgentThreadPool<AgentMCTS> * p, AgentMCTS * a) : AgentThreadBase<AgentMCTS>(p, a) { }


		void reset(){
			treelen.reset();
			gamelen.reset();
			use_explore = false;
			for(int a = 0; a < 4; a++)
				times[a] = 0;
		}


	private:
		void iterate(); //handles each iteration
		void walk_tree(Board & board, Node * node, int depth);
		bool create_children(const Board & board, Node * node);
		void add_knowledge(const Board & board, Node * node, Node * child);
		Node * choose_move(const Node * node, int toplay) const;

		int rollout(Board & board, Move move, int depth);
	};


public:

	static const float min_rave;

	bool  ponder;     //think during opponents time?
	int   numthreads; //number of player threads to run
	u64   maxmem;     //maximum memory for the tree in bytes
	bool  profile;    //count how long is spent in each stage of MCTS

//tree traversal
	bool  parentexplore; // whether to multiple exploration by the parents winrate
	float explore;    //greater than one favours exploration, smaller than one favours exploitation
	bool  knowledge;  //whether to include knowledge
	float useexplore; //what probability to use UCT exploration
	float fpurgency;  //what value to return for a move that hasn't been played yet
	int   rollouts;   //number of rollouts to run after the tree traversal

//tree building
	bool  keeptree;   //reuse the tree from the previous move
	int   minimax;    //solve the minimax tree within the uct tree
	uint  visitexpand;//number of visits before expanding a node
	bool  prunesymmetry; //prune symmetric children from the move list, useful for proving but likely not for playing
	uint  gcsolved;   //garbage collect solved nodes or keep them in the tree, assuming they meet the required amount of work

//knowledge
	int win_score;

//rollout
	int   instantwin;     //look for instant wins in rollouts

	Board rootboard;
	Node  root;
	uword nodes;
	int   gclimit; //the minimum experience needed to not be garbage collected

	uint64_t runs, maxruns;

	CompactTree<Node> ctmem;

	AgentThreadPool<AgentMCTS> pool;

	AgentMCTS();
	~AgentMCTS();

	void set_memlimit(uint64_t lim) { }; // in bytes
	void clear_mem() { };

	void set_ponder(bool p);
	void set_board(const Board & board, bool clear = true);

	void move(const Move & m);

	void search(double time, uint64_t maxruns, int verbose);
	Move return_move(int verbose) const { return return_move(& root, rootboard.toplay(), verbose); }

	double gamelen() const;
	vector<Move> get_pv() const;
	string move_stats(const vector<Move> moves) const;

	bool done() {
		//solved or finished runs
		return (rootboard.won() >= 0 || root.outcome >= 0 || (maxruns > 0 && runs >= maxruns));
	}

	bool need_gc() {
		//out of memory, start garbage collection
		return (ctmem.memalloced() >= maxmem);
	}

	void start_gc() {
		Time starttime;
		logerr("Starting player GC with limit " + to_str(gclimit) + " ... ");
		uint64_t nodesbefore = nodes;
		Board copy = rootboard;
		garbage_collect(copy, & root);
		Time gctime;
		ctmem.compact(1.0, 0.75);
		Time compacttime;
		logerr(to_str(100.0*nodes/nodesbefore, 1) + " % of tree remains - " +
			to_str((gctime - starttime)*1000, 0)  + " msec gc, " + to_str((compacttime - gctime)*1000, 0) + " msec compact\n");

		if(ctmem.meminuse() >= maxmem/2)
			gclimit = (int)(gclimit*1.3);
		else if(gclimit > rollouts*5)
			gclimit = (int)(gclimit*0.9); //slowly decay to a minimum of 5
	}


protected:

	void garbage_collect(Board & board, Node * node); //destroys the board, so pass in a copy
	bool do_backup(Node * node, Node * backup, int toplay);
	Move return_move(const Node * node, int toplay, int verbose = 0) const;
	Node * find_child(const Node * node, const Move & move) const ;
};
