
// This implements hex shape board.

#pragma once

#include "move.h"

namespace Morat {

template<class SubClassBoard>
class BoardShapeHex {
	const SubClassBoard* self() const {	return static_cast<const SubClassBoard*>(this);	}
	SubClassBoard* self() {	return static_cast<SubClassBoard*>(this);	}

public:
	//assumes x, y are in array bounds
	bool on_board_fast(int x, int y)   const { return (  y -   x < self()->size_r_) && (  x -   y < self()->size_r_); }
	bool on_board_fast(const Move & m) const { return (m.y - m.x < self()->size_r_) && (m.x - m.y < self()->size_r_); }
	//checks array bounds too
	bool on_board(int x, int y)  const { return (  x >= 0 &&   y >= 0 &&   x < self()->size_ &&   y < self()->size_ && self()->on_board_fast(x, y) ); }
	bool on_board(const Move & m)const { return (m.x >= 0 && m.y >= 0 && m.x < self()->size_ && m.y < self()->size_ && self()->on_board_fast(m) ); }
	bool on_board(const MoveValid & m) const { return m.on_board(); }

	int lines()           const { return self()->size_; }
	int line_start(int y) const { return (y < self()->size_r_ ? 0 : y - self()->size_r_m1_); }
	int line_end(int y)   const { return (y < self()->size_r_ ? self()->size_r_ + y : self()->size_); }
	int line_len(int y)   const { return self()->size_ - abs(self()->size_r_m1_ - y); }
};

}; // namespace Morat
