
#include <cmath>
#include <string>

#include "../lib/alarm.h"
#include "../lib/fileio.h"
#include "../lib/string.h"
#include "../lib/time.h"

#include "agentmcts.h"
#include "board.h"

const float AgentMCTS::min_rave = 0.1;

void AgentMCTS::search(double time, uint64_t max_runs, int verbose){
	int toplay = rootboard.toplay();

	if(rootboard.won() >= 0 || (time <= 0 && max_runs == 0))
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

			logerr("Win Types:   ");
			logerr("W: f " + to_str(wintypes[0][1].num*100.0/games,0) + "%, b " + to_str(wintypes[0][2].num*100.0/games,0) + "%, r " + to_str(wintypes[0][3].num*100.0/games,0) + "%; ");
			logerr("B: f " + to_str(wintypes[1][1].num*100.0/games,0) + "%, b " + to_str(wintypes[1][2].num*100.0/games,0) + "%, r " + to_str(wintypes[1][3].num*100.0/games,0) + "%\n");

			if(verbose >= 2){
				logerr("  W fork:    " + wintypes[0][1].to_s() + "\n");
				logerr("  W bridge:  " + wintypes[0][2].to_s() + "\n");
				logerr("  W ring:    " + wintypes[0][3].to_s() + "\n");
				logerr("  B fork:    " + wintypes[1][1].to_s() + "\n");
				logerr("  B bridge:  " + wintypes[1][2].to_s() + "\n");
				logerr("  B ring:    " + wintypes[1][3].to_s() + "\n");
			}
		}

		if(root.outcome != -3){
			logerr("Solved as a ");
			if(     root.outcome == 0)        logerr("draw\n");
			else if(root.outcome == 3)        logerr("draw by simultaneous win\n");
			else if(root.outcome == toplay)   logerr("win\n");
			else if(root.outcome == 3-toplay) logerr("loss\n");
			else if(root.outcome == -toplay)  logerr("win or draw\n");
			else if(root.outcome == toplay-3) logerr("loss or draw\n");
		}

		string pvstr;
		for(auto m : get_pv())
			pvstr += " " + m.to_s();
		logerr("PV:         " + pvstr + "\n");

		if(verbose >= 3 && !root.children.empty())
			logerr("Move stats:\n" + move_stats(vector<Move>()));
	}

	pool.reset();
	runs = 0;


	if(ponder && root.outcome < 0)
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
	detectdraw  = false;
	visitexpand = 1;
	prunesymmetry = false;
	gcsolved    = 100000;

	localreply  = 0;
	locality    = 0;
	connect     = 20;
	size        = 0;
	bridge      = 25;
	dists       = 0;

	weightedrandom = false;
	checkringdepth = 1000;
	ringperm       = 0;
	rolloutpattern = false;
	lastgoodreply  = false;
	instantwin     = 0;

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
	if(rootboard.won() < 0)
		root.outcome = -3;

	if(ponder)
		pool.resume();
}

double AgentMCTS::gamelen() const {
	DepthStats len;
	for(auto & t : pool)
		len += t->gamelen;
	return len.avg();
}

vector<Move> AgentMCTS::get_pv() const {
	vector<Move> pv;

	const Node * n = & root;
	char turn = rootboard.toplay();
	while(n && !n->children.empty()){
		Move m = return_move(n, turn);
		pv.push_back(m);
		n = find_child(n, m);
		turn = 3 - turn;
	}

	if(pv.size() == 0)
		pv.push_back(Move(M_RESIGN));

	return pv;
}

string AgentMCTS::move_stats(vector<Move> moves) const {
	string s = "";
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

Move AgentMCTS::return_move(const Node * node, int toplay, int verbose) const {
	if(node->outcome >= 0)
		return node->bestmove;

	double val, maxval = -1000000000000.0; //1 trillion

	Node * ret = NULL,
		 * child = node->children.begin(),
		 * end = node->children.end();

	for( ; child != end; child++){
		if(child->outcome >= 0){
			if(child->outcome == toplay) val =  800000000000.0 - child->exp.num(); //shortest win
			else if(child->outcome == 0) val = -400000000000.0 + child->exp.num(); //longest tie
			else                         val = -800000000000.0 + child->exp.num(); //longest loss
		}else{ //not proven
			if(msrave == -1) //num simulations
				val = child->exp.num();
			else if(msrave == -2) //num wins
				val = child->exp.sum();
			else
				val = child->value(msrave, 0, 0) - msexplore*sqrt(log(node->exp.num())/(child->exp.num() + 1));
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

	int toplay = board.toplay();
	for( ; child != end; child++){
		if(child->children.num() == 0)
			continue;

		if(	(node->outcome >= 0 && child->exp.num() > gcsolved && (node->outcome != toplay || child->outcome == toplay || child->outcome == 0)) || //parent is solved, only keep the proof tree, plus heavy draws
			(node->outcome <  0 && child->exp.num() > (child->outcome >= 0 ? gcsolved : gclimit)) ){ // only keep heavy nodes, with different cutoffs for solved and unsolved
			board.set(child->move);
			garbage_collect(board, child);
			board.unset(child->move);
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

void AgentMCTS::gen_hgf(Board & board, Node * node, unsigned int limit, unsigned int depth, FILE * fd){
	string s = string("\n") + string(depth, ' ') + "(;" + (board.toplay() == 2 ? "W" : "B") + "[" + node->move.to_s() + "]" +
	       "C[mcts, sims:" + to_str(node->exp.num()) + ", avg:" + to_str(node->exp.avg(), 4) + ", outcome:" + to_str((int)(node->outcome)) + ", best:" + node->bestmove.to_s() + "]";
	fprintf(fd, "%s", s.c_str());

	Node * child = node->children.begin(),
		 * end = node->children.end();

	int toplay = board.toplay();

	bool children = false;
	for( ; child != end; child++){
		if(child->exp.num() >= limit && (toplay != node->outcome || child->outcome == node->outcome) ){
			board.set(child->move);
			gen_hgf(board, child, limit, depth+1, fd);
			board.unset(child->move);
			children = true;
		}
	}

	if(children)
		fprintf(fd, "\n%s", string(depth, ' ').c_str());
	fprintf(fd, ")");
}

void AgentMCTS::create_children_simple(const Board & board, Node * node){
	assert(node->children.empty());

	node->children.alloc(board.movesremain(), ctmem);

	Node * child = node->children.begin(),
		 * end   = node->children.end();
	Board::MoveIterator moveit = board.moveit(prunesymmetry);
	int nummoves = 0;
	for(; !moveit.done() && child != end; ++moveit, ++child){
		*child = Node(*moveit);
		nummoves++;
	}

	if(prunesymmetry)
		node->children.shrink(nummoves); //shrink the node to ignore the extra moves
	else //both end conditions should happen in parallel
		assert(moveit.done() && child == end);

	PLUS(nodes, node->children.num());
}

//reads the format from gen_hgf.
void AgentMCTS::load_hgf(Board board, Node * node, FILE * fd){
	char c, buf[101];

	eat_whitespace(fd);

	assert(fscanf(fd, "(;%c[%100[^]]]", &c, buf) > 0);

	assert(board.toplay() == (c == 'W' ? 1 : 2));
	node->move = Move(buf);
	board.move(node->move);

	assert(fscanf(fd, "C[%100[^]]]", buf) > 0);

	vecstr entry, parts = explode(string(buf), ", ");
	assert(parts[0] == "mcts");

	entry = explode(parts[1], ":");
	assert(entry[0] == "sims");
	uword sims = from_str<uword>(entry[1]);

	entry = explode(parts[2], ":");
	assert(entry[0] == "avg");
	double avg = from_str<double>(entry[1]);

	uword wins = sims*avg;
	node->exp.addwins(wins);
	node->exp.addlosses(sims - wins);

	entry = explode(parts[3], ":");
	assert(entry[0] == "outcome");
	node->outcome = from_str<int>(entry[1]);

	entry = explode(parts[4], ":");
	assert(entry[0] == "best");
	node->bestmove = Move(entry[1]);


	eat_whitespace(fd);

	if(fpeek(fd) != ')'){
		create_children_simple(board, node);

		while(fpeek(fd) != ')'){
			Node child;
			load_hgf(board, & child, fd);

			Node * i = find_child(node, child.move);
			*i = child;          //copy the child experience to the tree
			i->swap_tree(child); //move the child subtree to the tree

			assert(child.children.empty());

			eat_whitespace(fd);
		}
	}

	eat_char(fd, ')');

	return;
}
