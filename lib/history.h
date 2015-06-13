
#pragma once

#include <vector>

#include "move.h"


namespace Morat {

template<class Board>
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

	int len() const {
		return hist.size();
	}

	void clear() {
		hist.clear();
		board.clear();
	}

	bool undo() {
		if(hist.size() <= 0)
			return false;

		hist.pop_back();

		board.clear();
		for(auto m : hist) {
			board.move(m);
		}
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

}; // namespace Morat
