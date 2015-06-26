
// This implements hex connectivity.

#pragma once

#include "board_base.h"

namespace Morat {

template<class SubClassBoard>
class BoardBaseHex : public BoardBase<SubClassBoard> {
	using BoardBase<SubClassBoard>::self;

public:
	const MoveValid* neighbors(const Move& m)      const { return neighbors(self()->xy(m)); }
	const MoveValid* neighbors(const MoveValid& m) const { return neighbors(m.xy); }
	const MoveValid* neighbors(int i) const { return self()->neighbor_list_.get() + i*18; }

	NeighborIterator neighbors_small(const Move& m)      const { return neighbors_small(self()->xy(m)); }
	NeighborIterator neighbors_small(const MoveValid& m) const { return neighbors_small(m.xy); }
	NeighborIterator neighbors_small(int i) const { return NeighborIterator(neighbors(i), 6); }

	NeighborIterator neighbors_medium(const Move& m)      const { return neighbors_medium(self()->xy(m)); }
	NeighborIterator neighbors_medium(const MoveValid& m) const { return neighbors_medium(m.xy); }
	NeighborIterator neighbors_medium(int i) const { return NeighborIterator(neighbors(i), 12); }

	NeighborIterator neighbors_large(const Move& m)      const { return neighbors_large(self()->xy(m)); }
	NeighborIterator neighbors_large(const MoveValid& m) const { return neighbors_large(m.xy); }
	NeighborIterator neighbors_large(int i) const { return NeighborIterator(neighbors(i), 18); }

	int local(const MoveValid & m, Side turn) const { return local(m.xy, turn); }
	int local(const Move & m,      Side turn) const { return local(self()->xy(m), turn); }
	int local(int i,               Side turn) const {
		Pattern p = pattern(i);
		Pattern x = ((p & 0xAAAAAAAAAull) >> 1) ^ (p & 0x555555555ull); // p1 is now when p1 or p2 but not both (ie off the board)
		p = x & (turn == Side::P1 ? p : p >> 1); // now just the selected player
		return (p & 0x000000FFF ? 3 : 0) |
		       (p & 0x000FFF000 ? 2 : 0) |
		       (p & 0xFFF000000 ? 1 : 0);
	}

	Pattern sympattern(const MoveValid & pos) const { return sympattern(pos.xy); }
	Pattern sympattern(const Move & pos)      const { return sympattern(self()->xy(pos)); }
	Pattern sympattern(int posxy)             const { return pattern_symmetry(pattern(posxy)); }

	Pattern pattern(const MoveValid & pos) const { return pattern(pos.xy); }
	Pattern pattern(const Move & pos)      const { return pattern(self()->xy(pos)); }
	Pattern pattern(int posxy)             const {
		// this is from the opposite perspective
		// so rotate into this move's perspective
		return pattern_reverse(self()->cells_[posxy].pattern);
	}

	Pattern pattern_medium(const MoveValid & pos) const { return pattern_medium(pos.xy); }
	Pattern pattern_medium(const Move & pos)      const { return pattern_medium(self()->xy(pos)); }
	Pattern pattern_medium(int posxy)             const {
		return pattern(posxy) & ((1ull << 24) - 1);
	}

	Pattern pattern_small(const MoveValid & pos) const { return pattern_small(pos.xy); }
	Pattern pattern_small(const Move & pos)      const { return pattern_small(self()->xy(pos)); }
	Pattern pattern_small(int posxy)             const {
		return pattern(posxy) & ((1ull << 12) - 1);
	}

	static Pattern pattern_reverse(Pattern p) { // switch perspectives (position out, or position in)
		return (((p & 0x03F03F03Full) << 6) | ((p & 0xFC0FC0FC0ull) >> 6));
	}

	static Pattern pattern_invert(Pattern p) { //switch players
		return ((p & 0xAAAAAAAAAull) >> 1) | ((p & 0x555555555ull) << 1);
	}
	static Pattern pattern_rotate(Pattern p) {
		return (((p & 0x003003003ull) << 10) | ((p & 0xFFCFFCFFCull) >> 2));
	}
	static Pattern pattern_mirror(Pattern p) {
		// HGFEDC BA9876 543210 -> DEFGHC 6789AB 123450
		return ((p & (3ull <<  6))      ) | ((p & (3ull <<  0))     ) | // 0,3 stay in place
		       ((p & (3ull << 10)) >>  8) | ((p & (3ull <<  2)) << 8) | // 1,5 swap
		       ((p & (3ull <<  8)) >>  4) | ((p & (3ull <<  4)) << 4) | // 2,4 swap
		       ((p & (3ull << 22)) >> 10) | ((p & (3ull << 12)) <<10) | // 6,B swap
		       ((p & (3ull << 20)) >>  6) | ((p & (3ull << 14)) << 6) | // 7,A swap
		       ((p & (3ull << 18)) >>  2) | ((p & (3ull << 16)) << 2) | // 8,9 swap
		       ((p & (3ull << 30))      ) | ((p & (3ull << 24))     ) | // F,C stay in place
		       ((p & (3ull << 34)) >>  8) | ((p & (3ull << 26)) << 8) | // H,D swap
		       ((p & (3ull << 32)) >>  4) | ((p & (3ull << 28)) << 4);  // G,E swap
	}
	static Pattern pattern_symmetry(Pattern p) { //takes a pattern and returns the representative version
		Pattern m = p;                 //012345
		m = std::min(m, (p = pattern_rotate(p)));//501234
		m = std::min(m, (p = pattern_rotate(p)));//450123
		m = std::min(m, (p = pattern_rotate(p)));//345012
		m = std::min(m, (p = pattern_rotate(p)));//234501
		m = std::min(m, (p = pattern_rotate(p)));//123450
		m = std::min(m, (p = pattern_mirror(pattern_rotate(p))));//012345 -> 054321
		m = std::min(m, (p = pattern_rotate(p)));//105432
		m = std::min(m, (p = pattern_rotate(p)));//210543
		m = std::min(m, (p = pattern_rotate(p)));//321054
		m = std::min(m, (p = pattern_rotate(p)));//432105
		m = std::min(m, (p = pattern_rotate(p)));//543210
		return m;
	}
};

}; // namespace Morat
