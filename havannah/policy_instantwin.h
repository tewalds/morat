
#pragma once

#include "../lib/assert2.h"
#include "../lib/move.h"

#include "board.h"
#include "policy.h"


namespace Morat {
namespace Havannah {

class InstantWin : public Policy {
	int max_rollout_moves;
	int cur_rollout_moves;

	Move saved_loss;
public:

	InstantWin() : max_rollout_moves(10), cur_rollout_moves(0), saved_loss(M_UNKNOWN) {
	}

	void rollout_start(Board & board, int max) {
		if(max < 0)
			max *= - board.get_size();
		max_rollout_moves = max;

		cur_rollout_moves = 0;
		saved_loss = M_UNKNOWN;
	}

	Move choose_move(const Board & board, const Move & prev) {
		if(saved_loss != M_UNKNOWN)
			return saved_loss;

		if(cur_rollout_moves++ >= max_rollout_moves)
			return M_UNKNOWN;

		//must have an edge connection, or it has nothing to offer a group towards a win
		const Board::Cell * c = board.cell(prev);
		if(c->numedges() == 0)
			return M_UNKNOWN;

		Move start, cur, loss = M_UNKNOWN;
		Side turn = ~board.toplay();

		//find the first empty cell
		int dir = -1;
		for(int i = 0; i <= 5; i++){
			start = prev + neighbours[i];

			if(!board.onboard(start) || board.get(start) != turn){
				dir = (i + 5) % 6;
				break;
			}
		}

		if(dir == -1) //possible if it's in the middle of a ring
			return M_UNKNOWN;

		cur = start;

//		logerr(board.to_s(true));
//		logerr(prev.to_s() + ":");

		//follow contour of the current group looking for wins
		do{
//			logerr(" " + cur.to_s());
			//check the current cell
			if(board.onboard(cur) && board.get(cur) == Side::NONE && board.test_outcome(cur, turn) == +turn){
//				logerr(" loss");
				if(loss == M_UNKNOWN){
					loss = cur;
				}else if(loss != cur){
					saved_loss = loss;
					return cur; //game over, two wins found for opponent
				}
			}

			//advance to the next cell
			for(int i = 5; i <= 9; i++){
				int nd = (dir + i) % 6;
				Move next = cur + neighbours[nd];

				if(!board.onboard(next) || board.get(next) != turn){
					cur = next;
					dir = nd;
					break;
				}
			}
		}while(cur != start); //potentially skips part of it when the start is in a pocket, rare bug

//		logerr("\n");

		return loss; // usually M_UNKNOWN
	}
};

}; // namespace Havannah
}; // namespace Morat
