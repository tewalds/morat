
#pragma once

//A Monte-Carlo Tree Search based player

#include <cmath>
#include <cassert>

#include "../lib/agentpool.h"
#include "../lib/compacttree.h"
#include "../lib/depthstats.h"
#include "../lib/exppair.h"
#include "../lib/log.h"
#include "../lib/move.h"
#include "../lib/movelist.h"
#include "../lib/policy_bridge.h"
#include "../lib/policy_instantwin.h"
#include "../lib/policy_lastgoodreply.h"
#include "../lib/policy_random.h"
#include "../lib/thread.h"
#include "../lib/time.h"
#include "../lib/types.h"
#include "../lib/xorshift.h"

#include "agent.h"
#include "board.h"
#include "lbdist.h"


namespace Morat {
namespace Hex {

class AgentMCTS : public Agent{
public:

	struct Node {
	public:
		ExpPair rave;
		ExpPair exp;
		int16_t know;
		Outcome outcome;
		uint8_t proofdepth;
		Move    move;
		Move    bestmove; //if outcome is set, then bestmove is the way to get there
		CompactTree<Node>::Children children;
//		int padding;
		//seems to need padding to multiples of 8 bytes or it segfaults?
		//don't forget to update the copy constructor/operator

		Node() : know(0), outcome(Outcome::UNKNOWN), proofdepth(0), move(M_NONE) { }
		Node(const Move & m, Outcome o = Outcome::UNKNOWN) : know(0), outcome(o), proofdepth(0), move(m) { }
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

		std::string to_s() const ;
		bool from_s(std::string s);

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


	class AgentThread : public AgentThreadBase<AgentMCTS> {
		mutable XORShift_float unitrand;
		LastGoodReply<Board> last_good_reply;
		RandomPolicy<Board> random_policy;
		ProtectBridge<Board> protect_bridge;
		InstantWin<Board> instant_wins;

		bool use_rave;    //whether to use rave for this simulation
		bool use_explore; //whether to use exploration for this simulation
		LBDists dists;    //holds the distances to the various non-ring wins as a heuristic for the minimum moves needed to win

		MoveList<Board> movelist;
		int stage; //which of the four MCTS stages is it on

	public:
		DepthStats treelen, gamelen;
		double times[4]; //time spent in each of the stages
		Time timestamps[4]; //timestamps for the beginning, before child creation, before rollout, after rollout

		AgentThread(AgentThreadPool<AgentMCTS> * p, AgentMCTS * a) : AgentThreadBase<AgentMCTS>(p, a) { }


		void reset(){
			treelen.reset();
			gamelen.reset();

			use_rave = false;
			use_explore = false;

			for(int a = 0; a < 4; a++)
				times[a] = 0;
		}


	private:
		void iterate(); //handles each iteration
		void walk_tree(Board & board, Node * node, int depth);
		bool create_children(const Board & board, Node * node);
		void add_knowledge(const Board & board, Node * node, Node * child);
		Node * choose_move(const Node * node, Side toplay, int remain) const;
		void update_rave(const Node * node, Side toplay);
		bool test_bridge_probe(const Board & board, const Move & move, const Move & test) const;

		Outcome rollout(Board & board, Move move, int depth);
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
	bool  longestloss;//tells us how to proceed if we have a proved lose, if true aim for the longest lose, if false try for hardest lose to solve

//knowledge
	int   localreply; //boost for a local reply, ie a move near the previous move
	int   locality;   //boost for playing near previous stones
	int   connect;    //boost for having connections to edges and corners
	int   size;       //boost for large groups
	int   bridge;     //boost replying to a probe at a bridge
	int   dists;      //boost based on minimum number of stones needed to finish a non-ring win
//rollout
	int   weightedrandom; //use weighted random for move ordering based on gammas
	bool  rolloutpattern; //play the response to a virtual connection threat in rollouts
	int   lastgoodreply;  //use the last-good-reply rollout heuristic
	int   instantwin;     //how deep to look for instant wins in rollouts

	float gammas[4096]; //pattern weights for weighted random

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
	vecmove get_pv() const;
	std::string move_stats(const vecmove moves) const;

	bool done() {
		//solved or finished runs
		return (rootboard.won() >= Outcome::DRAW || root.outcome >= Outcome::DRAW || (maxruns > 0 && runs >= maxruns));
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

	void gen_sgf(SGFPrinter<Move> & sgf, int limit) const {
		if(limit < 0)
			limit = root.exp.num()/1000;
		gen_sgf(sgf, limit, root, rootboard.toplay());
	}

	void load_sgf(SGFParser<Move> & sgf) {
		load_sgf(sgf, rootboard, root);
	}

protected:

	void garbage_collect(Board & board, Node * node); //destroys the board, so pass in a copy
	bool do_backup(Node * node, Node * backup, Side toplay);
	Move return_move(const Node * node, Side toplay, int verbose = 0) const;

	Node * find_child(const Node * node, const Move & move) const ;
	void create_children_simple(const Board & board, Node * node);

	void gen_sgf(SGFPrinter<Move> & sgf, unsigned int limit, const Node & node, Side side) const ;
	void load_sgf(SGFParser<Move> & sgf, const Board & board, Node & node);
};

}; // namespace Hex
}; // namespace Morat
