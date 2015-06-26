
// This implements oct connectivity.

#pragma once

#include "board_base.h"

namespace Morat {

template<class SubClassBoard>
class BoardBaseOct : public BoardBase<SubClassBoard> {
	using BoardBase<SubClassBoard>::self;

public:
	const MoveValid* neighbors(const Move& m)      const { return neighbors(self()->xy(m)); }
	const MoveValid* neighbors(const MoveValid& m) const { return neighbors(m.xy); }
	const MoveValid* neighbors(int i) const { return self()->neighbor_list_.get() + i*24; }

	NeighborIterator neighbors_small(const Move& m)      const { return neighbors_small(self()->xy(m)); }
	NeighborIterator neighbors_small(const MoveValid& m) const { return neighbors_small(m.xy); }
	NeighborIterator neighbors_small(int i) const { return NeighborIterator(neighbors(i), 8); }

	NeighborIterator neighbors_medium(const Move& m)      const { return neighbors_medium(self()->xy(m)); }
	NeighborIterator neighbors_medium(const MoveValid& m) const { return neighbors_medium(m.xy); }
	NeighborIterator neighbors_medium(int i) const { return NeighborIterator(neighbors(i), 16); }

	NeighborIterator neighbors_large(const Move& m)      const { return neighbors_large(self()->xy(m)); }
	NeighborIterator neighbors_large(const MoveValid& m) const { return neighbors_large(m.xy); }
	NeighborIterator neighbors_large(int i) const { return NeighborIterator(neighbors(i), 24); }

	int local(const MoveValid & m, Side turn) const { return local(m.xy, turn); }
	int local(const Move & m,      Side turn) const { return local(self()->xy(m), turn); }
	int local(int i,               Side turn) const {
		Pattern p = pattern(i);
		Pattern x = ((p & 0xAAAAAAAAAAAAull) >> 1) ^ (p & 0x555555555555ull); // p1 is now when p1 or p2 but not both (ie off the board)
		p = x & (turn == Side::P1 ? p : p >> 1); // now just the selected player
		return (p & 0x00000000FFFF ? 3 : 0) |
		       (p & 0x0000FFFF0000 ? 2 : 0) |
		       (p & 0xFFFF00000000 ? 1 : 0);
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
		return pattern(posxy) & ((1ull << 32) - 1);
	}

	Pattern pattern_small(const MoveValid & pos) const { return pattern_small(pos.xy); }
	Pattern pattern_small(const Move & pos)      const { return pattern_small(self()->xy(pos)); }
	Pattern pattern_small(int posxy)             const {
		return pattern(posxy) & ((1ull << 16) - 1);
	}

	static Pattern pattern_reverse(Pattern p) { // switch perspectives (position out, or position in)
		return ((p & 0x00FF00FF00FFull) << 8) | ((p & 0xFF00FF00FF00ull) >> 8);
	}

	static Pattern pattern_invert(Pattern p) { //switch players
		return ((p & 0xAAAAAAAAAAAAull) >> 1) | ((p & 0x555555555555ull) << 1);
	}
	static Pattern pattern_rotate(Pattern p) {
		return ((p & 0x000F000F000Full) << 12) | ((p & 0xFFF0FFF0FFF0ull) >> 4);
	}
	static Pattern pattern_mirror(Pattern p) {
		/*
		 *  8  16 9 17 10    10 17  9 16  8
		 * 23  0  1  2 18    18  2  1  0 23
		 * 15  7  X  3 11 => 11  3  X  7 15
		 * 22  6  5  4 19    19  4  5  6 22
		 * 14 21 13 20 12    12 20 13 21 14
		 */

		// pull out an entry
		#define e(a) (p & (3ull << (2 * a)))

		// swap two entries, a > b
		#define s(a, b) ((e(a) >> (2 * (a - b))) | e(b) << (2 * (a - b)))

		return e(9) | e(1) | e(5) | e(13) |
		       s(17, 16) | s( 2,  0) | s( 7,  3) | s( 6,  4) | s(21, 20) |
		       s(10,  8) | s(23, 18) | s(15, 11) | s(22, 19) | s(14, 12);

		#undef e
		#undef s
	}
	static Pattern pattern_symmetry(Pattern p) { //takes a pattern and returns the representative version
		Pattern m = p;                 //0123
		m = std::min(m, (p = pattern_rotate(p)));//3012
		m = std::min(m, (p = pattern_rotate(p)));//2301
		m = std::min(m, (p = pattern_rotate(p)));//3120
		m = std::min(m, (p = pattern_mirror(pattern_rotate(p))));//0123 -> 0321
		m = std::min(m, (p = pattern_rotate(p)));//1032
		m = std::min(m, (p = pattern_rotate(p)));//2103
		m = std::min(m, (p = pattern_rotate(p)));//3210
		return m;
	}
};

}; // namespace Morat
