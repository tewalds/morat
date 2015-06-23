
#include <cmath>
#include <string>

#include "../lib/assert2.h"
#include "../lib/string.h"

#include "agentmcts.h"


namespace Morat {
namespace Gomoku {

void AgentMCTS::AgentThread::iterate(){
	INCR(agent->runs);
	if(agent->profile){
		timestamps[0] = Time();
		stage = 0;
	}

	movelist.reset(&(agent->rootboard));
	agent->root.exp.addvloss();
	Board copy = agent->rootboard;
	use_rave    = (unitrand() < agent->userave);
	use_explore = (unitrand() < agent->useexplore);
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
			int remain = board.moves_remain();
			child = choose_move(node, to_play, remain);

			if(child->outcome < Outcome::DRAW){
				movelist.addtree(child->move, to_play);

				if(!board.move(child->move)){
					logerr("move failed: " + child->move.to_s() + "\n" + board.to_s(true));
					assert(false && "move failed");
				}

				child->exp.addvloss(); //balanced out after rollouts

				walk_tree(board, child, depth+1);

				child->exp.addv(movelist.getexp(to_play));

				if(!agent->do_backup(node, child, to_play) && //not solved
					agent->ravefactor > min_rave &&  //using rave
					node->children.num() > 1 &&       //not a macro move
					50*remain*(agent->ravefactor + agent->decrrave*remain) > node->exp.num()) //rave is still significant
					update_rave(node, to_play);

				return;
			}
		}while(!agent->do_backup(node, child, to_play));

		return;
	}

	if(agent->profile && stage == 0){
		stage = 1;
		timestamps[1] = Time();
	}

	Outcome won = (agent->minimax ? node->outcome : board.outcome());

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
		random_policy.prepare(board);
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

	Side to_play = board.to_play();
	Side opponent = ~to_play;
	int losses = 0;

	Node * child = temp.begin(),
	     * loss  = NULL;
	for (auto move : board) {
		*child = Node(move);

		if(agent->minimax){
			child->outcome = board.test_outcome(move);

			if(agent->minimax >= 2 && board.test_outcome(move, opponent) == +opponent){
				losses++;
				loss = child;
			}

			if(child->outcome == +to_play){ //proven win from here, don't need children
				node->outcome = child->outcome;
				node->proofdepth = 1;
				node->bestmove = move;
				node->children.unlock();
				temp.dealloc(agent->ctmem);
				return true;
			}
		}

		if(agent->knowledge)
			add_knowledge(board, node, child);
		child++;
	}
	assert(child == temp.end());

	//Make a macro move, add experience to the move so the current simulation continues past this move
	if(losses == 1){
		Node macro = *loss;
		temp.dealloc(agent->ctmem);
		temp.alloc(1, agent->ctmem);
		macro.exp.addwins(agent->visitexpand);
		*(temp.begin()) = macro;
	}else if(losses >= 2){ //proven loss, but at least try to block one of them
		node->outcome = +opponent;
		node->proofdepth = 2;
		node->bestmove = loss->move;
		node->children.unlock();
		temp.dealloc(agent->ctmem);
		return true;
	}

	if(agent->dynwiden > 0) //sort in decreasing order by knowledge
		std::sort(temp.begin(), temp.end(), sort_node_know);

	PLUS(agent->nodes, temp.num());
	node->children.swap(temp);
	assert(temp.unlock());

	return true;
}

AgentMCTS::Node * AgentMCTS::AgentThread::choose_move(const Node * node, Side to_play, int remain) const {
	float val, maxval = -1000000000;
	float logvisits = log(node->exp.num());
	int dynwidenlim = (agent->dynwiden > 0 ? (int)(logvisits/agent->logdynwiden)+2 : Board::max_vec_size);

	float raveval = use_rave * (agent->ravefactor + agent->decrrave*remain);
	float explore = use_explore * agent->explore;
	if(agent->parentexplore)
		explore *= node->exp.avg();

	Node * ret   = NULL,
		 * child = node->children.begin(),
		 * end   = node->children.end();

	for(; child != end && dynwidenlim >= 0; child++){
		if(child->outcome >= Outcome::DRAW){
			if(child->outcome == to_play) //return a win immediately
				return child;

			val = (child->outcome == Outcome::DRAW ? -1 : -2); //-1 for tie so any unknown is better, -2 for loss so it's even worse
		}else{
			val = child->value(raveval, agent->knowledge, agent->fpurgency);
			if(explore > 0)
				val += explore*sqrt(logvisits/(child->exp.num() + 1));
			dynwidenlim--;
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
3 win/draw/loss, ie unknown
2 draw
1 draw/loss
0 lose
return true if fully solved, false if it's unknown or partially unknown
*/
bool AgentMCTS::do_backup(Node * node, const Node * backup, Side to_play){
	Outcome node_outcome = node->outcome;
	if(node_outcome >= Outcome::DRAW) //already proven, probably by a different thread
		return true;

	if(backup->outcome == Outcome::UNKNOWN) //nothing proven by this child, so no chance
		return false;

	uint8_t proofdepth = backup->proofdepth;
	if(backup->outcome != to_play){
		int best_outcome = 0;
		backup = NULL;

		for (const auto& child : node->children) {
			Outcome child_outcome = child.outcome; //save a copy to avoid race conditions
			int outcome = 0;

			//these should be sorted in likelyness of matching, most likely first
			if(child_outcome == Outcome::UNKNOWN){ // win/draw/loss
				outcome = 3;
			}else if(child_outcome == to_play){ //win
				backup = &child;
				best_outcome = 6;
				proofdepth = child.proofdepth;
				break;
			}else if(child_outcome == ~to_play){ //loss
				outcome = 0;
			}else if(child_outcome == Outcome::DRAW ||
			         child_outcome == Outcome::DRAW2){ //draw
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

			if(proofdepth < child.proofdepth)
				proofdepth = child.proofdepth;

			auto better_than_best = [&]() -> bool {
				// any child is better than nothing
				if (backup == NULL)
					return true;

				// a better outcome is always preferable
				if (best_outcome < outcome)
					return true;
				if (best_outcome > outcome)
					return false;

				// do we care about depth? if so, take the longest.
				if (longestloss) {
					if (backup->proofdepth < child.proofdepth)
						return true;
					if (backup->proofdepth > child.proofdepth)
						return false;
				}

				// all else equal, take the hardest to solve
				return (backup->exp.num() < child.exp.num());
			};

			if (better_than_best()) {
				best_outcome = outcome;
				backup = &child;
			}
		}

		if(best_outcome == 3) //no win, but found an unknown
			return false;
	}

	if(node->outcome.cas(node_outcome, backup->outcome)){
		node->bestmove = backup->move;
		node->proofdepth = proofdepth + 1;
	}else //if it was in a race, try again, might promote a partial solve to full solve
		return do_backup(node, backup, to_play);

	return (node->outcome >= Outcome::DRAW);
}

//update the rave score of all children that were played
void AgentMCTS::AgentThread::update_rave(const Node * node, Side to_play){
	Node * child = node->children.begin(),
	     * childend = node->children.end();

	for( ; child != childend; ++child)
		child->rave.addv(movelist.getrave(to_play, child->move));
}


void AgentMCTS::AgentThread::add_knowledge(const Board & board, Node * node, Node * child){
	if(agent->localreply){ //boost for moves near the previous move
		int dist = board.dist(node->move, child->move);
		if(dist < 4)
			child->know += agent->localreply * (4 - dist);
	}

	if(agent->locality) //boost for moves near previous stones
		child->know += agent->locality * board.local(child->move, board.to_play());
}

///////////////////////////////////////////


//play a random game starting from a board state, and return the results of who won
Outcome AgentMCTS::AgentThread::rollout(Board & board, Move move, int depth){
	Outcome won;

	random_policy.rollout_start(board);

	while((won = board.outcome()) < Outcome::DRAW){
		Side turn = board.to_play();

		move = rollout_choose_move(board, move);

		movelist.addrollout(move, turn);

		assert2(board.move(move, true, false), "\n" + board.to_s(true) + "\n" + move.to_s());
		depth++;
	}

	gamelen.add(depth);

	if(won > Outcome::DRAW)
		win_types[won.to_i() - 1][(int)board.win_type()].add(depth);

	//update the last good reply table
	if(agent->lastgoodreply)
		last_good_reply.rollout_end(board, movelist, won);

	movelist.finishrollout(won);
	return won;
}

Move AgentMCTS::AgentThread::rollout_choose_move(Board & board, const Move & prev){
	//reuse the last good reply
	if(agent->lastgoodreply){
		Move move = last_good_reply.choose_move(board, prev);
		if(move != M_UNKNOWN)
			return move;
	}

	return random_policy.choose_move(board, prev);
}

}; // namespace Gomoku
}; // namespace Morat
