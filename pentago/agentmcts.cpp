
#include <cmath>
#include <string>

#include "../lib/alarm.h"
#include "../lib/fileio.h"
#include "../lib/string.h"
#include "../lib/time.h"

#include "agentmcts.h"
#include "board.h"


namespace Morat {
namespace Pentago {

const float AgentMCTS::min_rave = 0.1;

std::string AgentMCTS::Node::to_s() const {
	return "AgentMCTS::Node"
	       ", move " + move.to_s() +
	       ", exp " + exp.to_s() +
	       ", know " + to_str(know) +
	       ", outcome " + to_str((int)outcome.to_i()) +
	       ", depth " + to_str((int)proofdepth) +
	       ", best " + bestmove.to_s() +
	       ", children " + to_str(children.num());
}

bool AgentMCTS::Node::from_s(std::string s) {
	auto dict = parse_dict(s, ", ", " ");

	if(dict.size() == 8){
		move = Move(dict["move"]);
		exp = ExpPair(dict["exp"]);
		know = from_str<int>(dict["know"]);
		outcome = Outcome(from_str<int>(dict["outcome"]));
		proofdepth = from_str<int>(dict["depth"]);
		bestmove = Move(dict["best"]);
		// ignore children
		return true;
	}
	return false;
}

void AgentMCTS::search(double time, uint64_t max_runs, int verbose){
	Side to_play = rootboard.to_play();

	if(rootboard.won() >= Outcome::DRAW || (time <= 0 && max_runs == 0))
		return;

	Time starttime;

	pool.pause();

	if(runs)
		logerr("Pondered " + to_str(runs) + " runs\n");

	runs = 0;
	maxruns = max_runs;
	pool.reset();

	//let them run!
	pool.resume();

	pool.wait_pause(time);

	double time_used = Time() - starttime;


	if(verbose){
		DepthStats gamelen, treelen;
		double times[4] = {0,0,0,0};
		for(auto & t : pool){
			gamelen += t->gamelen;
			treelen += t->treelen;

			for(int a = 0; a < 4; a++)
				times[a] += t->times[a];
		}

		logerr("Finished:    " + to_str(runs) + " runs in " + to_str(time_used*1000, 0) + " msec: " + to_str(runs/time_used, 0) + " Games/s\n");
		if(runs > 0){
			logerr("Game length: " + gamelen.to_s() + "\n");
			logerr("Tree depth:  " + treelen.to_s() + "\n");
			if(profile)
				logerr("Times:       " + to_str(times[0], 3) + ", " + to_str(times[1], 3) + ", " + to_str(times[2], 3) + ", " + to_str(times[3], 3) + "\n");
		}

		if(root.outcome != Outcome::UNKNOWN)
			logerr("Solved as a " + root.outcome.to_s_rel(to_play) + "\n");

		std::string pvstr;
		for(auto m : get_pv())
			pvstr += " " + m.to_s();
		logerr("PV:         " + pvstr + "\n");

		if(verbose >= 3 && !root.children.empty())
			logerr("Move stats:\n" + move_stats(vecmove()));
	}

	pool.reset();
	runs = 0;


	if(ponder && root.outcome < Outcome::DRAW)
		pool.resume();
}

AgentMCTS::AgentMCTS() : pool(this) {
	nodes = 0;
	runs = 0;
	gclimit = 5;

	profile     = false;
	ponder      = false;
//#ifdef SINGLE_THREAD ... make sure only 1 thread
	numthreads  = 1;
	pool.set_num_threads(numthreads);
	maxmem      = 1000*1024*1024;

	explore     = 1;
	parentexplore = true;
	knowledge   = true;
	useexplore  = 1;
	fpurgency   = 1;
	rollouts    = 10;

	keeptree    = true;
	minimax     = 1;
	visitexpand = 1;
	prunesymmetry = true;
	gcsolved    = 100000;

	win_score = 1;

	instantwin  = 0;

}
AgentMCTS::~AgentMCTS(){
	pool.pause();
	pool.set_num_threads(0);

	root.dealloc(ctmem);
	ctmem.compact();
}

void AgentMCTS::set_ponder(bool p){
	if(ponder != p){
		ponder = p;
		pool.pause();

		if(ponder)
			pool.resume();
	}
}

void AgentMCTS::set_board(const Board & board, bool clear){
	pool.pause();

	nodes -= root.dealloc(ctmem);
	root = Node();
	root.exp.addwins(visitexpand+1);

	rootboard = board;

	if(ponder)
		pool.resume();
}
void AgentMCTS::move(const Move & m){
	pool.pause();

	uword nodesbefore = nodes;

	if(keeptree && root.children.num() > 0){
		Node child;

		for(Node * i = root.children.begin(); i != root.children.end(); i++){
			if(i->move == m){
				child = *i;          //copy the child experience to temp
				child.swap_tree(*i); //move the child tree to temp
				break;
			}
		}

		nodes -= root.dealloc(ctmem);
		root = child;
		root.swap_tree(child);

		if(nodesbefore > 0)
			logerr("Nodes before: " + to_str(nodesbefore) + ", after: " + to_str(nodes) + ", saved " +  to_str(100.0*nodes/nodesbefore, 1) + "% of the tree\n");
	}else{
		nodes -= root.dealloc(ctmem);
		root = Node();
		root.move = m;
	}
	assert(nodes == root.size());

	rootboard.move(m);

	root.exp.addwins(visitexpand+1); //+1 to compensate for the virtual loss
	if(rootboard.won() < Outcome::DRAW)
		root.outcome = Outcome::UNKNOWN;

	if(ponder)
		pool.resume();
}

double AgentMCTS::gamelen() const {
	DepthStats len;
	for(auto & t : pool)
		len += t->gamelen;
	return len.avg();
}

std::vector<Move> AgentMCTS::get_pv() const {
	vecmove pv;

	const Node * n = & root;
	Side turn = rootboard.to_play();
	while(n && !n->children.empty()){
		Move m = return_move(n, turn);
		pv.push_back(m);
		n = find_child(n, m);
		turn = ~turn;
	}

	if(pv.size() == 0)
		pv.push_back(Move(M_RESIGN));

	return pv;
}

std::string AgentMCTS::move_stats(vecmove moves) const {
	std::string s = "";
	const Node * node = & root;

	if(moves.size()){
		s += "path:\n";
		for(auto m : moves){
			if(node){
				node = find_child(node, m);
				s += node->to_s() + "\n";
			}
		}
	}

	if(node){
		s += "children:\n";
		for(auto & n : node->children)
			s += n.to_s() + "\n";
	}
	return s;
}

Move AgentMCTS::return_move(const Node * node, Side to_play, int verbose) const {
	if(node->outcome >= Outcome::DRAW)
		return node->bestmove;

	double val, maxval = -1000000000000.0; //1 trillion

	Node * ret = NULL,
		 * child = node->children.begin(),
		 * end = node->children.end();

	for( ; child != end; child++){
		if(child->outcome >= Outcome::DRAW){
			if(child->outcome == to_play)             val =  800000000000.0 - child->exp.num(); //shortest win
			else if(child->outcome == Outcome::DRAW) val = -400000000000.0 + child->exp.num(); //longest tie
			else                                     val = -800000000000.0 + child->exp.num(); //longest loss
		}else{ //not proven
//			val = child->exp.num(); //num simulations
			val = child->exp.sum(); //num wins
		}

		if(maxval < val){
			maxval = val;
			ret = child;
		}
	}

	assert(ret);

	if(verbose)
		logerr("Score:       " + to_str(ret->exp.avg()*100., 2) + "% / " + to_str(ret->exp.num()) + "\n");

	return ret->move;
}

void AgentMCTS::garbage_collect(Board & board, Node * node){
	Node * child = node->children.begin(),
		 * end = node->children.end();

	Side to_play = board.to_play();
	for( ; child != end; child++){
		if(child->children.num() == 0)
			continue;

		if(	(node->outcome >= Outcome::DRAW && child->exp.num() > gcsolved && (node->outcome != to_play || child->outcome == to_play || child->outcome == Outcome::DRAW)) || //parent is solved, only keep the proof tree, plus heavy draws
			(node->outcome <  Outcome::DRAW && child->exp.num() > (child->outcome >= Outcome::DRAW ? gcsolved : gclimit)) ){ // only keep heavy nodes, with different cutoffs for solved and unsolved
			board.move(child->move);
			garbage_collect(board, child);
			board.undo(child->move);
		}else{
			nodes -= child->dealloc(ctmem);
		}
	}
}

AgentMCTS::Node * AgentMCTS::find_child(const Node * node, const Move & move) const {
	for(Node * i = node->children.begin(); i != node->children.end(); i++)
		if(i->move == move)
			return i;

	return NULL;
}

void AgentMCTS::gen_sgf(SGFPrinter<Move> & sgf, unsigned int limit, const Node & node, Side side) const {
	for(auto & child : node.children){
		if(child.exp.num() >= limit && (side != node.outcome || child.outcome == node.outcome)){
			sgf.child_start();
			sgf.move(side, child.move);
			sgf.comment(child.to_s());
			gen_sgf(sgf, limit, child, ~side);
			sgf.child_end();
		}
	}
}

void AgentMCTS::create_children_simple(const Board & board, Node * node){
	assert(node->children.empty());

	node->children.alloc(board.moves_avail(), ctmem);

	Node * child = node->children.begin(),
		 * end   = node->children.end();
	MoveIterator moveit(board, prunesymmetry);
	int num_moves = 0;
	for(; !moveit.done() && child != end; ++moveit, ++child){
		*child = Node(*moveit);
		num_moves++;
	}

	if(prunesymmetry)
		node->children.shrink(num_moves); //shrink the node to ignore the extra moves
	else //both end conditions should happen in parallel
		assert(moveit.done() && child == end);

	PLUS(nodes, node->children.num());
}

void AgentMCTS::load_sgf(SGFParser<Move> & sgf, const Board & board, Node & node) {
	assert(sgf.has_children());
	create_children_simple(board, & node);

	while(sgf.next_child()){
		Move m = sgf.move();
		Node & child = *find_child(&node, m);
		child.from_s(sgf.comment());
		if(sgf.done_child()){
			continue;
		}else{
			// has children!
			Board b = board;
			b.move(m);
			load_sgf(sgf, b, child);
			assert(sgf.done_child());
		}
	}
}

}; // namespace Pentago
}; // namespace Morat
