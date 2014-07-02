
#include "../lib/alarm.h"
#include "../lib/log.h"
#include "../lib/time.h"

#include "agentpns.h"


namespace Morat {
namespace Hex {

std::string AgentPNS::Node::to_s() const {
	return "AgentPNS::Node"
	       ", move " + move.to_s() +
	       ", phi " + to_str(phi) +
	       ", delta " + to_str(delta) +
	       ", work " + to_str(work) +
	       ", children " + to_str(children.num());
}

bool AgentPNS::Node::from_s(std::string s) {
	auto dict = parse_dict(s, ", ", " ");

	if(dict.size() == 6){
		move = Move(dict["move"]);
		phi = from_str<uint32_t>(dict["phi"]);
		delta = from_str<uint32_t>(dict["delta"]);
		work = from_str<uint64_t>(dict["work"]);
		// ignore children
		return true;
	}
	return false;
}

void AgentPNS::test() {
	Node n(Move("a1"));
	auto s = n.to_s();
	Node k;
	assert(k.from_s(s));
}

void AgentPNS::search(double time, uint64_t maxiters, int verbose){
	max_nodes_seen = maxiters;

	if(rootboard.won() >= 0)
		return;

	Time starttime;

	pool.reset();
	pool.resume();

	pool.wait_pause(time);


	double time_used = Time() - starttime;


	if(verbose){
		DepthStats treelen;
		for(auto & t : pool)
			treelen += t->treelen;

		logerr("Finished:    " + to_str(nodes_seen) + " nodes created in " + to_str(time_used*1000, 0) + " msec: " + to_str(nodes_seen/time_used, 0) + " Nodes/s\n");
		if(nodes_seen > 0){
			logerr("Tree depth:  " + treelen.to_s() + "\n");
		}

		Side toplay = rootboard.toplay();

		logerr("Root:        " + root.to_s() + "\n");
		Outcome outcome = root.to_outcome(~toplay);
		if(outcome != Outcome::UNKNOWN)
			logerr("Solved as a " + outcome.to_s_rel(toplay) + "\n");

		std::string pvstr;
		for(auto m : get_pv())
			pvstr += " " + m.to_s();
		logerr("PV:         " + pvstr + "\n");

		if(verbose >= 3 && !root.children.empty())
			logerr("Move stats:\n" + move_stats(vecmove()));
	}
}

void AgentPNS::AgentThread::iterate(){
	pns(agent->rootboard, &agent->root, 0, INF32/2, INF32/2);
}

bool AgentPNS::AgentThread::pns(const Board & board, Node * node, int depth, uint32_t tp, uint32_t td){
	// no children, create them
	if(node->children.empty()){
		treelen.add(depth);

		if(node->terminal())
			return true;

		if(agent->need_gc())
			return false;

		if(!node->children.lock())
			return false;

		int numnodes = board.movesremain();
		CompactTree<Node>::Children temp;
		temp.alloc(numnodes, agent->ctmem);

		if(agent->lbdist)
			dists.run(&board);

		unsigned int i = 0;
		for(Board::MoveIterator move = board.moveit(true); !move.done(); ++move){
			unsigned int pd;
			Outcome outcome;

			if(agent->ab){
				Board next = board;
				next.move(*move);

				pd = 0;
				outcome = (agent->ab == 1 ? solve1ply(next, pd) : solve2ply(next, pd));
			}else{
				pd = 1;
				outcome = board.test_outcome(*move);
			}

			if(agent->lbdist && outcome != Outcome::UNKNOWN)
				pd = dists.get(*move);

			temp[i] = Node(*move).outcome(outcome, board.toplay(), agent->ties, pd);
			i++;
		}
		nodes_seen += i;
		PLUS(agent->nodes_seen, i);
		PLUS(agent->nodes, i);
		temp.shrink(i); //if symmetry, there may be extra moves to ignore
		node->children.swap(temp);
		assert(temp.unlock());

		updatePDnum(node);

		return (agent->nodes_seen >= agent->max_nodes_seen);
	}

	bool mem;
	do{
		Node * child = node->children.begin(), // the best move to explore
		     * child2 = node->children.begin();// second best for thresholds

		uint32_t tpc, tdc; // the thresholds

		if(agent->df){
			for(auto & i : node->children){
				if(i.refdelta() <= child->refdelta()){
					child2 = child;
					child = & i;
				}else if(i.refdelta() < child2->refdelta()){
					child2 = & i;
				}
			}

			tpc = std::min(INF32/2, (td + child->phi - node->delta));
			tdc = std::min(tp, (uint32_t)(child2->delta*(1.0 + agent->epsilon) + 1));
		}else{
			tpc = tdc = 0;
			for(auto & i : node->children)
				if(child->refdelta() > i.refdelta())
					child = & i;
		}

		Board next = board;
		next.move(child->move);

		child->ref();
		uint64_t seen_before = nodes_seen;
		mem = pns(next, child, depth + 1, tpc, tdc);
		child->deref();
		PLUS(child->work, nodes_seen - seen_before);

		if(updatePDnum(node) && !agent->df)
			break;

	}while(!agent->timeout && mem && (!agent->df || (node->phi < tp && node->delta < td)));

	return mem;
}

bool AgentPNS::AgentThread::updatePDnum(Node * node){
	Node * i = node->children.begin();
	Node * end = node->children.end();

	uint32_t min = i->delta;
	uint64_t sum = 0;

	bool win = false;
	for( ; i != end; i++){
		win |= (i->phi == LOSS);
		sum += i->phi;
		if( min > i->delta)
			min = i->delta;
	}

	if(win)
		sum = LOSS;
	else if(sum >= INF32)
		sum = INF32;

	if(min == node->phi && sum == node->delta){
		return false;
	}else{
		if(sum == 0 && min == DRAW){
			node->phi = 0;
			node->delta = DRAW;
		}else{
			node->phi = min;
			node->delta = sum;
		}
		return true;
	}
}


double AgentPNS::gamelen() const {
	//TODO: how to calculate this?
	return rootboard.movesremain();
}

std::vector<Move> AgentPNS::get_pv() const {
	vecmove pv;

	const Node * n = & root;
	Side turn = rootboard.toplay();
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

std::string AgentPNS::move_stats(vecmove moves) const {
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

Move AgentPNS::return_move(const Node * node, Side toplay, int verbose) const {
	double val, maxval = -1000000000000.0; //1 trillion

	Node * ret = NULL,
		 * child = node->children.begin(),
		 * end = node->children.end();

	for( ; child != end; child++){
		Outcome outcome = child->to_outcome(toplay);
		if(outcome >= Outcome::DRAW){
			if(     outcome == +toplay)       val =  800000000000.0 - (double)child->work; //shortest win
			else if(outcome == Outcome::DRAW) val = -400000000000.0 + (double)child->work; //longest tie
			else                              val = -800000000000.0 + (double)child->work; //longest loss
		}else{ //not proven
			val = child->work;
		}

		if(maxval < val){
			maxval = val;
			ret = child;
		}
	}

	assert(ret);

	if(verbose)
		logerr(ret->to_s() + "\n");

	return ret->move;
}

AgentPNS::Node * AgentPNS::find_child(const Node * node, const Move & move) const {
	for(auto & n : node->children)
		if(n.move == move)
			return &n;

	return NULL;
}

//removes the children of any node with less than limit work
void AgentPNS::garbage_collect(Node * node){
	Node * child = node->children.begin();
	Node * end = node->children.end();

	for( ; child != end; child++){
		if(child->terminal() || child->work < gclimit){ //solved or low work, ignore solvedness since it's trivial to re-solve
			nodes -= child->dealloc(ctmem);
		}else if(child->children.num() > 0){
			garbage_collect(child);
		}
	}
}

void AgentPNS::create_children_simple(const Board & board, Node * node){
	assert(node->children.empty());
	node->children.alloc(board.movesremain(), ctmem);
	unsigned int i = 0;
	for(Board::MoveIterator move = board.moveit(true); !move.done(); ++move){
		Outcome outcome = board.test_outcome(*move);
		node->children[i] = Node(*move).outcome(outcome, board.toplay(), ties, 1);
		i++;
	}
	PLUS(nodes, i);
	node->children.shrink(i); //if symmetry, there may be extra moves to ignore
}

void AgentPNS::gen_sgf(SGFPrinter<Move> & sgf, unsigned int limit, const Node & node, Side side) const {
	for(auto & child : node.children){
		if(child.work >= limit && (side != node.to_outcome(~side) || child.to_outcome(side) == node.to_outcome(~side))){
			sgf.child_start();
			sgf.move(side, child.move);
			sgf.comment(child.to_s());
			gen_sgf(sgf, limit, child, ~side);
			sgf.child_end();
		}
	}
}

void AgentPNS::load_sgf(SGFParser<Move> & sgf, const Board & board, Node & node) {
	assert(sgf.has_children());
	create_children_simple(board, &node);

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

}; // namespace Hex
}; // namespace Morat
