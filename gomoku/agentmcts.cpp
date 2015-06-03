
#include <cmath>
#include <string>

#include "../lib/alarm.h"
#include "../lib/fileio.h"
#include "../lib/string.h"
#include "../lib/time.h"

#include "agentmcts.h"
#include "board.h"


namespace Morat {
namespace Gomoku {

const float AgentMCTS::min_rave = 0.1;

std::string AgentMCTS::Node::to_s() const {
	return "AgentMCTS::Node"
	       ", move " + move.to_s() +
	       ", exp " + exp.to_s() +
	       ", rave " + rave.to_s() +
	       ", know " + to_str(know) +
	       ", outcome " + to_str((int)outcome.to_i()) +
	       ", depth " + to_str((int)proofdepth) +
	       ", best " + bestmove.to_s() +
	       ", children " + to_str(children.num());
}

bool AgentMCTS::Node::from_s(std::string s) {
	auto dict = parse_dict(s, ", ", " ");

	if(dict.size() == 9){
		move = Move(dict["move"]);
		exp = ExpPair(dict["exp"]);
		rave = ExpPair(dict["rave"]);
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

	if(rootboard.outcome() >= Outcome::DRAW || (time <= 0 && max_runs == 0))
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
		DepthStats wintypes[2][4];
		uint64_t games = 0;
		double times[4] = {0,0,0,0};
		for(auto & t : pool){
			gamelen += t->gamelen;
			treelen += t->treelen;

			for(int a = 0; a < 2; a++){
				for(int b = 0; b < 4; b++){
					wintypes[a][b] += t->wintypes[a][b];
					games += t->wintypes[a][b].num;
				}
			}

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
		for(const auto& m : Agent::get_pv())
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

	msrave      = -2;
	msexplore   = 0;

	explore     = 0;
	parentexplore = false;
	ravefactor  = 500;
	decrrave    = 200;
	knowledge   = true;
	userave     = 1;
	useexplore  = 1;
	fpurgency   = 1;
	rollouts    = 1;
	dynwiden    = 0;
	logdynwiden = (dynwiden ? std::log(dynwiden) : 0);

	shortrave   = false;
	keeptree    = true;
	minimax     = 2;
	visitexpand = 1;
	prunesymmetry = false;
	gcsolved    = 100000;

	localreply  = 0;
	locality    = 0;

	weightedrandom = false;
	lastgoodreply  = false;

	for(int i = 0; i < 4096; i++)
		gammas[i] = 1;
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
	if(rootboard.outcome() < Outcome::DRAW)
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

std::vector<Move> AgentMCTS::get_pv(const vecmove& moves) const {
	vecmove pv;

	const Node * n = & root;
	Side turn = rootboard.to_play();
	int i = 0;
	while(n && !n->children.empty()){
		Move m = (i < moves.size() ? moves[i++] : return_move(n, turn));
		pv.push_back(m);
		n = find_child(n, m);
		turn = ~turn;
	}

	if(pv.size() == 0)
		pv.push_back(Move(M_RESIGN));

	return pv;
}

std::string AgentMCTS::move_stats(const vecmove& moves) const {
	std::string s;
	const Node * node = & root;

	s += "root:\n";
	s += node->to_s() + "\n";

	if(moves.size()){
		s += "path:\n";
		for(const auto& m : moves){
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

	assert(!node->children.empty());

	double val, maxval = -1000000000000.0; //1 trillion

	const Node * ret = NULL;
	for(const auto& child : node->children) {
		if(child.outcome >= Outcome::DRAW){
			if(child.outcome == to_play)            val =  800000000000.0 - child.exp.num(); //shortest win
			else if(child.outcome == Outcome::DRAW) val = -400000000000.0 + child.exp.num(); //longest tie
			else                                    val = -800000000000.0 + child.exp.num(); //longest loss
		}else{ //not proven
			if(msrave == -1) //num simulations
				val = child.exp.num();
			else if(msrave == -2) //num wins
				val = child.exp.sum();
			else
				val = child.value(msrave, 0, 0) - msexplore*sqrt(log(node->exp.num())/(child.exp.num() + 1));
		}

		if(maxval < val){
			maxval = val;
			ret = &child;
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
			board.set(child->move);
			garbage_collect(board, child);
			board.unset(child->move);
		}else{
			nodes -= child->dealloc(ctmem);
		}
	}
}

AgentMCTS::Node * AgentMCTS::find_child(const Node * node, const Move & move) const {
	for(auto & c : node->children)
		if(c.move == move)
			return &c;
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

	node->children.alloc(board.moves_remain(), ctmem);

	Node * child = node->children.begin(),
	     * end   = node->children.end();
	for (auto move : board) {  // TODO: prunesymmetry
		assert(child != end);
		*child++ = Node(move);
	}

	// if(prunesymmetry)
	// 	node->children.shrink(nummoves); //shrink the node to ignore the extra moves
	// else //both end conditions should happen in parallel
	assert(child == end);

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

}; // namespace Gomoku
}; // namespace Morat
