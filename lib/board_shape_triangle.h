
// This implements triangle shaped board.

#pragma once

#include "move.h"

namespace Morat {

template<class SubClassBoard>
class BoardShapeTriangle {
	const SubClassBoard* self() const {	return static_cast<const SubClassBoard*>(this);	}
	SubClassBoard* self() {	return static_cast<SubClassBoard*>(this);	}

public:
	//assumes x, y are in array bounds
	bool on_board_fast(int x, int y)   const { return (  y +   x < self()->size_); }
	bool on_board_fast(const Move & m) const { return (m.y + m.x < self()->size_); }
	//checks array bounds too
	bool on_board(int x, int y)  const { return (  x >= 0 &&   y >= 0 && on_board_fast(x, y) ); }
	bool on_board(const Move & m)const { return (m.x >= 0 && m.y >= 0 && on_board_fast(m) ); }
	bool on_board(const MoveValid & m) const { return m.on_board(); }

	int lines()           const { return self()->size_; }
	int line_start(int y) const { return 0; }
	int line_end(int y)   const { return (self()->size_ - y); }
	int line_len(int y)   const { return line_end(y) - line_start(y); }
};

}; // namespace Morat
