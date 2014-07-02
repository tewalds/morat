
# pragma once

#include "board.h"
#include "move.h"
#include "policy.h"


namespace Morat {
namespace Hex {

class LastGoodReply : public Policy {
	Move goodreply[2][Board::max_vecsize];
	int enabled;
public:

	LastGoodReply(int _enabled = 2) : enabled(_enabled) {
		for(int p = 0; p < 2; p++)
			for(int i = 0; i < Board::max_vecsize; i++)
				goodreply[p][i] = M_UNKNOWN;
	}

	Move choose_move(const Board & board, const Move & prev) const {
		if (enabled && prev != M_SWAP) {
			Move move = goodreply[board.toplay().to_i() - 1][board.xy(prev)];
			if(move != M_UNKNOWN && board.valid_move_fast(move))
				return move;
		}
		return M_UNKNOWN;
	}

	void rollout_end(const Board & board, const MoveList & movelist, Outcome outcome) {
		if(!enabled || outcome != Outcome::DRAW)
			return;
		int m = -1;
		for(const MovePlayer * i = movelist.begin(), * e = movelist.end(); i != e; i++){
			if(m >= 0){
				if(+i->player == outcome && *i != M_SWAP)
					goodreply[i->player.to_i() - 1][m] = *i;
				else if(enabled == 2)
					goodreply[i->player.to_i() - 1][m] = M_UNKNOWN;
			}
			m = board.xy(*i);
		}
	}
};

}; // namespace Hex
}; // namespace Morat
