
#pragma once

#include <vector>

#include "../lib/move.h"
#include "../lib/xorshift.h"

#include "policy.h"


namespace Morat {

template<class Board>
class RandomPolicy : public Policy<Board> {
	XORShift_uint32 rand;
	Move moves[Board::max_vecsize];
	int num;
	int cur;
public:

	RandomPolicy() : num(0), cur(0) {
	}

	// only need to save the valid moves once since all the rollouts start from the same position
	void prepare(const Board & board) {
		num = 0;
		for (auto m : board)
			moves[num++] = m;
	}

	// reset the set of moves to make from above. Since they're used in random order they don't need to be in iterator order
	void rollout_start(Board & board) {
		cur = num;
	}

	Move choose_move(const Board & board, const Move & prev) {
		while(true){
			int r = rand() % cur;
			cur--;
			Move m = moves[r];
			moves[r] = moves[cur];
			moves[cur] = m;
			if(board.valid_move_fast(m))
				return m;
		}
	}
};

}; // namespace Morat
