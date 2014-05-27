
# pragma once

#include "board.h"
#include "move.h"
#include "policy.h"

class LastGoodReply : public Policy {
	Move goodreply[2][256]; //256 is big enough for size 16, but no bigger...
	int enabled;
public:

	LastGoodReply(int _enabled = 2) : enabled(_enabled) {
		for(int p = 0; p < 2; p++)
			for(int i = 0; i < 256; i++)
				goodreply[p][i] = M_UNKNOWN;
	}

	Move choose_move(const Board & board, const Move & prev) const {
		/*if (enabled && prev != M_SWAP) {
			Move move = goodreply[board.toplay()-1][board.xy(prev)];
			if(move != M_UNKNOWN && board.valid_move_fast(move))
				return move;
		}*/
		return M_UNKNOWN;
	}

	void rollout_end(const Board & board, const MoveList & movelist, int won) {
		if(!enabled)
			return;
		int m = -1;
		for(const MovePlayer * i = movelist.begin(), * e = movelist.end(); i != e; i++){
			if(m >= 0){
				if(i->player == won && *i != M_SWAP)
					goodreply[i->player - 1][m] = *i;
				else if(enabled == 2)
					goodreply[i->player - 1][m] = M_UNKNOWN;
			}
			m = board.xy(*i);
		}
	}
};
