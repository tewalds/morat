
#include "base_board.h"
#include "move_iterator.h"

namespace Morat {

MoveIterator & MoveIterator::operator ++ () {
	do {
		move.x++;
		move.xy++;

		if (move.x >= lineend) {
			move.y++;
			if (move.y >= board.size_y()) {  // done
				move.xy = -1;
				return *this;
			}
			move.x = board.line_start(move.y);
			move.xy = board.xy(move.x, move.y);
			lineend = board.line_end(move.y);
		}
	} while (!board.valid_move_fast(move));

	return *this;
}


MoveIteratorUnique::MoveIteratorUnique(const BaseBoard & b) : MoveIterator(b) {
	hashes.init(b.moves_avail());
}

MoveIteratorUnique & MoveIteratorUnique::operator ++ () { //prefix form
	while (true) {
		MoveIterator::operator++();
		if (move == M_NONE) {
			return *this;
		}

		auto h = board.test_hash(move, board.to_play());
		if (hashes.add(h)) {
			return *this;
		}
	}
}
/*
template<class Rand>
MoveIteratorRandom::MoveIteratorRandom(const BaseBoard & b, Rand & r)
		: MoveIterator(b), rand(r) {
	moves.reserve(b.moves_avail());
	for(auto m : b) {
		moves.push_back(m);
	}
	MoveIterator::operator++();
}

template<class Rand>
MoveIteratorRandom & MoveIteratorRandom::operator ++ () { //prefix form
	auto len = moves.length();
	if (len > 0) {
		// incremental fisher-yates shuffle. Choose a random element that hasn't yet been chosen.
		int i = rand() % len;
		move = moves[i];
		moves[i] = moves[len - 1];
		moves.pop_back();
	} else {
		move = M_NONE;
	}
	return *this;
}
*/
}; // namespace Morat
