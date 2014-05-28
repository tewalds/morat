
#pragma once

#include <vector>

#include "../lib/string.h"

#include "board.h"
#include "move.h"

class History {
	std::vector<Move> hist;
	Board board;

public:

	History() { }
	History(const Board & b) : board(b) { }

	const Move & operator [] (int i) const {
		return hist[i];
	}

	Move last() const {
		if(hist.size() == 0)
			return M_NONE;

		return hist.back();
	}

	const Board & operator * ()  const { return board; }
	const Board * operator -> () const { return & board; }

	std::vector<Move>::const_iterator begin() const { return hist.begin(); }
	std::vector<Move>::const_iterator end()   const { return hist.end(); }

	const Board get_board(int offset = 0) const {
		Board b(board.get_size());
		if(offset < 0)
			offset += hist.size();
		if(offset <= 0)
			return b;
		if(offset >= (int)hist.size())
			return board;
		for(int i = 0; i < offset; i++)
			b.move(hist[i]);
		return b;
	}

	int len() const {
		return hist.size();
	}

	void clear() {
		hist.clear();
		board = get_board();
	}

	bool undo() {
		if(hist.size() <= 0)
			return false;

		hist.pop_back();
		board = get_board();
		return true;
	}

	bool move(const Move & m) {
		if(board.valid_move(m)){
			board.move(m);
			hist.push_back(m);
			return true;
		}
		return false;
	}
};
