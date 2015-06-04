
#include <cmath>
#include <string>

#include "../lib/string.h"

#include "agentmcts.h"
#include "moveiterator.h"


namespace Morat {
namespace Pentago {

void AgentMCTS::AgentThread::iterate(){
	INCR(agent->runs);
	if(agent->profile){
		timestamps[0] = Time();
		stage = 0;
	}

	movelist.reset(&(agent->rootboard));
	agent->root.exp.addvloss();
	Board copy = agent->rootboard;
	walk_tree(copy, & agent->root, 0);
	agent->root.exp.addv(movelist.getexp(~agent->rootboard.to_play()));

	if(agent->profile){
		times[0] += timestamps[1] - timestamps[0];
		times[1] += timestamps[2] - timestamps[1];
		times[2] += timestamps[3] - timestamps[2];
		times[3] += Time() - timestamps[3];
	}
}

void AgentMCTS::AgentThread::walk_tree(Board & board, Node * node, int depth){
	Side to_play = board.to_play();

	if(!node->children.empty() && node->outcome < Outcome::DRAW){
	//choose a child and recurse
		Node * child;
		do{
			child = choose_move(node, to_play);

			if(child->outcome < Outcome::DRAW){
				movelist.addtree(child->move, to_play);

				if(!board.move(child->move)){
					logerr("move failed: " + child->move.to_s() + "\n" + board.to_s(true));
					assert(false && "move failed");
				}

				child->exp.addvloss(); //balanced out after rollouts

				walk_tree(board, child, depth+1);

				child->exp.addv(movelist.getexp(to_play));
				agent->do_backup(node, child, to_play);
				return;
			}
		}while(!agent->do_backup(node, child, to_play));

		return;
	}

	if(agent->profile && stage == 0){
		stage = 1;
		timestamps[1] = Time();
	}

	Outcome won = (agent->minimax ? node->outcome : board.won());

	//if it's not already decided
	if(won < Outcome::DRAW){
		//create children if valid
		if(node->exp.num() >= agent->visitexpand+1 && create_children(board, node)){
			walk_tree(board, node, depth);
			return;
		}

		if(agent->profile){
			stage = 2;
			timestamps[2] = Time();
		}

		//do random game on this node
		for(int i = 0; i < agent->rollouts; i++){
			Board copy = board;
			rollout(copy, node->move, depth);
		}
	}else{
		movelist.finishrollout(won); //got to a terminal state, it's worth recording
	}

	treelen.add(depth);

	movelist.subvlosses(1);

	if(agent->profile){
		timestamps[3] = Time();
		if(stage == 1)
			timestamps[2] = timestamps[3];
		stage = 3;
	}

	return;
}

bool sort_node_know(const AgentMCTS::Node & a, const AgentMCTS::Node & b){
	return (a.know > b.know);
}

bool AgentMCTS::AgentThread::create_children(const Board & board, Node * node){
	if(!node->children.lock())
		return false;

	CompactTree<Node>::Children temp;
	temp.alloc(board.moves_avail(), agent->ctmem);

	Node * child = temp.begin(),
	     * end   = temp.end();
	MoveIterator move(board, agent->prunesymmetry);
	int num_moves = 0;
	for(; !move.done() && child != end; ++move, ++child){
		*child = Node(*move);
		const Board & after = move.board();

		if(agent->minimax){
			child->outcome = after.won();

			if(child->outcome == board.to_play()){ //proven win from here, don't need children
				node->outcome = child->outcome;
				node->proofdepth = 1;
				node->bestmove = *move;
				node->children.unlock();
				temp.dealloc(agent->ctmem);
				return true;
			}
		}

		if(agent->knowledge)
			add_knowledge(after, node, child);
		num_moves++;
	}

	if(agent->prunesymmetry)
		temp.shrink(num_moves); //shrink the node to ignore the extra moves
	else{ //both end conditions should happen in parallel
		assert(move.done() && child == end);
	}

	//sort in decreasing order by knowledge
//	sort(temp.begin(), temp.end(), sort_node_know);

	PLUS(agent->nodes, temp.num());
	node->children.swap(temp);
	assert(temp.unlock());

	return true;
}

AgentMCTS::Node * AgentMCTS::AgentThread::choose_move(const Node * node, Side to_play) const {
	float val, maxval = -1000000000;
	float logvisits = log(node->exp.num());

	float explore = agent->explore;
	if(agent->parentexplore)
		explore *= node->exp.avg();

	Node * ret   = NULL,
		 * child = node->children.begin(),
		 * end   = node->children.end();

	for(; child != end; child++){
		if(child->outcome >= Outcome::DRAW){
			if(child->outcome == to_play) //return a win immediately
				return child;

			val = (child->outcome == Outcome::DRAW ? -1 : -2); //-1 for tie so any unknown is better, -2 for loss so it's even worse
		}else{
			val = child->value(agent->knowledge, agent->fpurgency);
			if(explore > 0)
				val += explore*sqrt(logvisits/(child->exp.num() + 1));
		}

		if(maxval < val){
			maxval = val;
			ret = child;
		}
	}

	return ret;
}

/*
backup in this order:

6 win
5 win/draw
4 draw if draw/loss
3 win/draw/loss
2 draw
1 draw/loss
0 lose
return true if fully solved, false if it's unknown or partially unknown
*/
bool AgentMCTS::do_backup(Node * node, Node * backup, Side to_play){
	Outcome node_outcome = node->outcome;
	if(node_outcome >= Outcome::DRAW) //already proven, probably by a different thread
		return true;

	if(backup->outcome == Outcome::UNKNOWN) //nothing proven by this child, so no chance
		return false;


	uint8_t proofdepth = backup->proofdepth;
	if(backup->outcome != to_play){
		uint64_t sims = 0, bestsims = 0, outcome = 0, best_outcome = 0;
		backup = NULL;

		Node * child = node->children.begin(),
			 * end = node->children.end();

		for( ; child != end; child++){
			Outcome child_outcome = child->outcome; //save a copy to avoid race conditions

			if(proofdepth < child->proofdepth+1)
				proofdepth = child->proofdepth+1;

			//these should be sorted in likelyness of matching, most likely first
			if(child_outcome == Outcome::UNKNOWN){ // win/draw/loss
				outcome = 3;
			}else if(child_outcome == to_play){ //win
				backup = child;
				outcome = 6;
				proofdepth = child->proofdepth+1;
				break;
			}else if(child_outcome == ~to_play){ //loss
				outcome = 0;
			}else if(child_outcome == Outcome::DRAW){ //draw
				if(node_outcome == -to_play) //draw/loss, ie I can't win
					outcome = 4;
				else
					outcome = 2;
			}else if(child_outcome == -~to_play){ //win/draw, ie opponent can't win
				outcome = 5;
			}else if(child_outcome == -to_play){ //draw/loss, ie I can't win
				outcome = 1;
			}else{
				logerr("child_outcome == " + child_outcome.to_s() + "\n");
				assert(false && "How'd I get here? All outcomes should be tested above");
			}

			sims = child->exp.num();
			if(best_outcome < outcome){ //better outcome is always preferable
				best_outcome = outcome;
				bestsims = sims;
				backup = child;
			}else if(best_outcome == outcome && ((outcome == 0 && bestsims < sims) || bestsims > sims)){
				//find long losses or easy wins/draws
				bestsims = sims;
				backup = child;
			}
		}

		if(best_outcome == 3) //no win, but found an unknown
			return false;
	}

	if(node->outcome.cas(node_outcome, backup->outcome)){
		node->bestmove = backup->move;
		node->proofdepth = proofdepth;
	}else //if it was in a race, try again, might promote a partial solve to full solve
		return do_backup(node, backup, to_play);

	return (node->outcome >= Outcome::DRAW);
}

void AgentMCTS::AgentThread::add_knowledge(const Board & board, Node * node, Node * child){
	if(agent->win_score > 0)
		child->know = agent->win_score * board.score_calc();
}

///////////////////////////////////////////


//play a random game starting from a board state, and return the results of who won
Outcome AgentMCTS::AgentThread::rollout(Board & board, Move move, int depth){
	Outcome won;
	while((won = board.won()) < Outcome::DRAW) {
		board.move_rand(rand64);
	}
	gamelen.add(board.moves_made());
	movelist.finishrollout(won);
	return won;
}

}; // namespace Pentago
}; // namespace Morat
