
#include <cmath>
#include <string>

#include "../lib/string.h"

#include "agentmcts.h"

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
	agent->root.exp.addv(movelist.getexp(3-agent->rootboard.toplay()));

	if(agent->profile){
		times[0] += timestamps[1] - timestamps[0];
		times[1] += timestamps[2] - timestamps[1];
		times[2] += timestamps[3] - timestamps[2];
		times[3] += Time() - timestamps[3];
	}
}

void AgentMCTS::AgentThread::walk_tree(Board & board, Node * node, int depth){
	int toplay = board.toplay();

	if(!node->children.empty() && node->outcome < 0){
	//choose a child and recurse
		Node * child;
		do{
			int remain = board.movesremain();
			child = choose_move(node, toplay, remain);

			if(child->outcome < 0){
				movelist.addtree(child->move, toplay);

				if(!board.move(child->move)){
					logerr("move failed: " + child->move.to_s() + "\n" + board.to_s(true));
					assert(false && "move failed");
				}

				child->exp.addvloss(); //balanced out after rollouts

				walk_tree(board, child, depth+1);

				child->exp.addv(movelist.getexp(toplay));

				if(!agent->do_backup(node, child, toplay) && //not solved
					agent->ravefactor > min_rave &&  //using rave
					node->children.num() > 1 &&       //not a macro move
					50*remain*(agent->ravefactor + agent->decrrave*remain) > node->exp.num()) //rave is still significant
					update_rave(node, toplay);

				return;
			}
		}while(!agent->do_backup(node, child, toplay));

		return;
	}

	if(agent->profile && stage == 0){
		stage = 1;
		timestamps[1] = Time();
	}

	int won = (agent->minimax ? node->outcome : board.won());

	//if it's not already decided
	if(won < 0){
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

	if(agent->dists || agent->detectdraw){
		dists.run(&board, (agent->dists > 0), (agent->detectdraw ? 0 : board.toplay()));

		if(agent->detectdraw){
//			assert(node->outcome == -3);
			node->outcome = dists.isdraw(); //could be winnable by only one side

			if(node->outcome == 0){ //proven draw, neither side can influence the outcome
				node->bestmove = *(board.moveit()); //just choose the first move since all are equal at this point
				node->children.unlock();
				return true;
			}
		}
	}

	CompactTree<Node>::Children temp;
	temp.alloc(board.movesremain(), agent->ctmem);

	int losses = 0;

	Node * child = temp.begin(),
	     * end   = temp.end(),
	     * loss  = NULL;
	Board::MoveIterator move = board.moveit(agent->prunesymmetry);
	int nummoves = 0;
	for(; !move.done() && child != end; ++move, ++child){
		*child = Node(*move);

		if(agent->minimax){
			child->outcome = board.test_win(*move);

			if(agent->minimax >= 2 && board.test_win(*move, 3 - board.toplay()) > 0){
				losses++;
				loss = child;
			}

			if(child->outcome == board.toplay()){ //proven win from here, don't need children
				node->outcome = child->outcome;
				node->proofdepth = 1;
				node->bestmove = *move;
				node->children.unlock();
				temp.dealloc(agent->ctmem);
				return true;
			}
		}

		if(agent->knowledge)
			add_knowledge(board, node, child);
		nummoves++;
	}

	if(agent->prunesymmetry)
		temp.shrink(nummoves); //shrink the node to ignore the extra moves
	else //both end conditions should happen in parallel
		assert(move.done() && child == end);

	//Make a macro move, add experience to the move so the current simulation continues past this move
	if(losses == 1){
		Node macro = *loss;
		temp.dealloc(agent->ctmem);
		temp.alloc(1, agent->ctmem);
		macro.exp.addwins(agent->visitexpand);
		*(temp.begin()) = macro;
	}else if(losses >= 2){ //proven loss, but at least try to block one of them
		node->outcome = 3 - board.toplay();
		node->proofdepth = 2;
		node->bestmove = loss->move;
		node->children.unlock();
		temp.dealloc(agent->ctmem);
		return true;
	}

	if(agent->dynwiden > 0) //sort in decreasing order by knowledge
		sort(temp.begin(), temp.end(), sort_node_know);

	PLUS(agent->nodes, temp.num());
	node->children.swap(temp);
	assert(temp.unlock());

	return true;
}

AgentMCTS::Node * AgentMCTS::AgentThread::choose_move(const Node * node, int toplay, int remain) const {
	float val, maxval = -1000000000;
	float logvisits = log(node->exp.num());
	int dynwidenlim = (agent->dynwiden > 0 ? (int)(logvisits/agent->logdynwiden)+2 : Board::max_vecsize);

	float raveval = use_rave * (agent->ravefactor + agent->decrrave*remain);
	float explore = use_explore * agent->explore;
	if(agent->parentexplore)
		explore *= node->exp.avg();

	Node * ret   = NULL,
		 * child = node->children.begin(),
		 * end   = node->children.end();

	for(; child != end && dynwidenlim >= 0; child++){
		if(child->outcome >= 0){
			if(child->outcome == toplay) //return a win immediately
				return child;

			val = (child->outcome == 0 ? -1 : -2); //-1 for tie so any unknown is better, -2 for loss so it's even worse
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
3 win/draw/loss
2 draw
1 draw/loss
0 lose
return true if fully solved, false if it's unknown or partially unknown
*/
bool AgentMCTS::do_backup(Node * node, Node * backup, int toplay){
	int nodeoutcome = node->outcome;
	if(nodeoutcome >= 0) //already proven, probably by a different thread
		return true;

	if(backup->outcome == -3) //nothing proven by this child, so no chance
		return false;


	uint8_t proofdepth = backup->proofdepth;
	if(backup->outcome != toplay){
		uint64_t sims = 0, bestsims = 0, outcome = 0, bestoutcome = 0;
		backup = NULL;

		Node * child = node->children.begin(),
			 * end = node->children.end();

		for( ; child != end; child++){
			int childoutcome = child->outcome; //save a copy to avoid race conditions

			if(proofdepth < child->proofdepth+1)
				proofdepth = child->proofdepth+1;

			//these should be sorted in likelyness of matching, most likely first
			if(childoutcome == -3){ // win/draw/loss
				outcome = 3;
			}else if(childoutcome == toplay){ //win
				backup = child;
				outcome = 6;
				proofdepth = child->proofdepth+1;
				break;
			}else if(childoutcome == 3-toplay){ //loss
				outcome = 0;
			}else if(childoutcome == 0){ //draw
				if(nodeoutcome == toplay-3) //draw/loss
					outcome = 4;
				else
					outcome = 2;
			}else if(childoutcome == -toplay){ //win/draw
				outcome = 5;
			}else if(childoutcome == toplay-3){ //draw/loss
				outcome = 1;
			}else{
				logerr("childoutcome == " + to_str(childoutcome) + "\n");
				assert(false && "How'd I get here? All outcomes should be tested above");
			}

			sims = child->exp.num();
			if(bestoutcome < outcome){ //better outcome is always preferable
				bestoutcome = outcome;
				bestsims = sims;
				backup = child;
			}else if(bestoutcome == outcome && ((outcome == 0 && bestsims < sims) || bestsims > sims)){
				//find long losses or easy wins/draws
				bestsims = sims;
				backup = child;
			}
		}

		if(bestoutcome == 3) //no win, but found an unknown
			return false;
	}

	if(CAS(node->outcome, nodeoutcome, backup->outcome)){
		node->bestmove = backup->move;
		node->proofdepth = proofdepth;
	}else //if it was in a race, try again, might promote a partial solve to full solve
		return do_backup(node, backup, toplay);

	return (node->outcome >= 0);
}

//update the rave score of all children that were played
void AgentMCTS::AgentThread::update_rave(const Node * node, int toplay){
	Node * child = node->children.begin(),
	     * childend = node->children.end();

	for( ; child != childend; ++child)
		child->rave.addv(movelist.getrave(toplay, child->move));
}


void AgentMCTS::AgentThread::add_knowledge(const Board & board, Node * node, Node * child){
	if(agent->localreply){ //boost for moves near the previous move
		int dist = node->move.dist(child->move);
		if(dist < 4)
			child->know += agent->localreply * (4 - dist);
	}

	if(agent->locality) //boost for moves near previous stones
		child->know += agent->locality * board.local(child->move, board.toplay());

	Board::Cell cell;
	if(agent->connect || agent->size)
		cell = board.test_cell(child->move);

	if(agent->connect) //boost for moves that connect to edges/corners
		child->know += agent->connect * (cell.numcorners() + cell.numedges());

	if(agent->size) //boost for size of the group
		child->know += agent->size * cell.size;

	if(agent->bridge && test_bridge_probe(board, node->move, child->move)) //boost for maintaining a virtual connection
		child->know += agent->bridge;

	if(agent->dists)
		child->know += abs(agent->dists) * max(0, board.get_size_d() - dists.get(child->move, board.toplay()));
}

//test whether this move is a forced reply to the opponent probing your virtual connections
bool AgentMCTS::AgentThread::test_bridge_probe(const Board & board, const Move & move, const Move & test) const {
	//TODO: switch to the same method as policy_bridge.h, maybe even share code
	if(move.dist(test) != 1)
		return false;

	bool equals = false;

	int state = 0;
	int piece = 3 - board.get(move);
	for(int i = 0; i < 8; i++){
		Move cur = move + neighbours[i % 6];

		bool on = board.onboard(cur);
		int v = 0;
		if(on)
			v = board.get(cur);

	//state machine that progresses when it see the pattern, but counting borders as part of the pattern
		if(state == 0){
			if(!on || v == piece)
				state = 1;
			//else state = 0;
		}else if(state == 1){
			if(on){
				if(v == 0){
					state = 2;
					equals = (test == cur);
				}else if(v != piece)
					state = 0;
				//else (v==piece) => state = 1;
			}
			//else state = 1;
		}else{ // state == 2
			if(!on || v == piece){
				if(equals)
					return true;
				state = 1;
			}else{
				state = 0;
			}
		}
	}
	return false;
}

///////////////////////////////////////////


//play a random game starting from a board state, and return the results of who won
int AgentMCTS::AgentThread::rollout(Board & board, Move move, int depth){
	int won;

	if(agent->instantwin)
		instant_wins.rollout_start(board, agent->instantwin);

	random_policy.rollout_start(board);

	//only check rings to the specified depth
	int  checkdepth = (int)agent->checkringdepth;
	//if it's negative, check for that fraction of the remaining moves
	if(agent->checkringdepth < 0)
		checkdepth = (int)ceil(board.movesremain() * agent->checkringdepth * -1);

	board.perm_rings = agent->ringperm;

	while((won = board.won()) < 0){
		int turn = board.toplay();

		board.check_rings = (depth < checkdepth);

		move = rollout_choose_move(board, move);

		movelist.addrollout(move, turn);

		assert2(board.move(move, true, false), "\n" + board.to_s(true) + "\n" + move.to_s());
		depth++;
	}

	gamelen.add(depth);

	if(won > 0)
		wintypes[won-1][(int)board.getwintype()].add(depth);

	//update the last good reply table
	if(agent->lastgoodreply)
		last_good_reply.rollout_end(board, movelist, won);

	movelist.finishrollout(won);
	return won;
}

Move AgentMCTS::AgentThread::rollout_choose_move(Board & board, const Move & prev){
	//look for instant wins
	if(agent->instantwin){
		Move move = instant_wins.choose_move(board, prev);
		if(move != M_UNKNOWN)
			return move;
	}

	//force a bridge reply
	if(agent->rolloutpattern){
		Move move = protect_bridge.choose_move(board, prev);
		if(move != M_UNKNOWN)
			return move;
	}

	//reuse the last good reply
	if(agent->lastgoodreply){
		Move move = last_good_reply.choose_move(board, prev);
		if(move != M_UNKNOWN)
			return move;
	}

	return random_policy.choose_move(board, prev);
}
