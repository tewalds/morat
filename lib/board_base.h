
// This is the base board structure that all games should inherit from. It
// implements some general and default things.
//
// It implements the basics of patterns, and should be subclassed for the
// different geometries, eg: hex connectivity (havannah, hex, y), quad
// connectivity (go), or oct connectivity (gomoku, connect 4).

#pragma once

#include "move.h"
#include "outcome.h"
#include "types.h"

namespace Morat {


template<class SubClassBoard>
class BoardBase {
protected:
	const SubClassBoard* self() const {	return static_cast<const SubClassBoard*>(this);	}
	SubClassBoard* self() {	return static_cast<SubClassBoard*>(this);	}

	void update_pattern(const MoveValid& pos, Side turn) {
		Pattern p = turn.to_i();
		for (auto m : self()->neighbors_large(pos)) {
			if(m.on_board()){
				self()->cells_[m.xy].pattern |= p;
			}
			p <<= 2;
		}
	}

	Pattern init_pattern(const MoveValid& pos) {
		Pattern p = 0, j = 3;
		for (const MoveValid m : self()->neighbors_large(pos)) {
			if (!m.on_board())
				p |= j;
			j <<= 2;
		}
		return self()->pattern_reverse(p);
	}

public:
	MoveValid move_valid(std::string s) const { return move_valid(Move(s)); }
	MoveValid move_valid(int x, int y) const { return move_valid(Move(x, y)); }
	MoveValid move_valid(Move m) const { return MoveValid(m, self()->xy(m)); }

	bool test_local(const Move & pos, Side turn) const { return test_local(self()->move_valid(pos), turn); }
	bool test_local(const MoveValid & pos, Side turn) const {
		return (self()->local(pos, turn) == 3);
	}
};

} // namespace Morat
