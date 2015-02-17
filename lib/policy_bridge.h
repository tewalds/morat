

#pragma once

#include "../lib/bits.h"
#include "../lib/move.h"

#include "policy.h"


namespace Morat {

template<class Board>
class ProtectBridge : public Policy<Board> {
	int offset;
	uint8_t lookup[2][1<<12];  // 2 players, all possible local 6-patterns

public:

	ProtectBridge() : offset(0) {
		// precompute the valid moves around a pattern for all possible 6-patterns.
		for(unsigned int i = 0; i < 1<<12; i++){
			lookup[0][i] = lookup[1][i] = 0;
			unsigned int p = i;
			for(unsigned int d = 0; d < 6; d++){
				// player 1
				if((p & 0x1D) == 0x11) // 01 11 01 -> 01 00 01
					lookup[0][i] |= (1 << ((d+1)%6)); // +1 because we want to play in the empty spot

				// player 2
				if((p & 0x2E) == 0x22) // 10 11 10 -> 10 00 10
					lookup[1][i] |= (1 << ((d+1)%6));

				p = ((p & 0xFFC)>>2) | ((p & 0x3) << 10);
			}
		}
	}

	Move choose_move(const Board & board, const Move & prev) {
		uint32_t p = board.pattern_small(prev);
		uint16_t r = lookup[board.toplay().to_i()-1][p];

		if(!r) // nothing to save
			return M_UNKNOWN;

		unsigned int i;
		if((r & (r - 1)) == 0){ // only one bit set
			i = trailing_zeros(r);
		} else { // multiple choices of bridges to save
			offset = (offset + 1) % 6; // rotate the starting offset to avoid directional bias
			r |= (r << 6);
			r >>= offset;
			i = (offset + trailing_zeros(r)) % 6;
		}
		return board.nb_begin(prev)[i];
	}
};

}; // namespace Morat
