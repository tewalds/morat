
#pragma once

// #include "hashset.h"
#include "move.h"
#include "types.h"

namespace Morat {

//iterator through neighbors of a position
class NeighborIterator {
	const MoveValid* begin_;
	const MoveValid* end_;
public:
	NeighborIterator(const MoveValid* begin, int count) : begin_(begin), end_(begin_ + count) {}
	const MoveValid* begin() const { return begin_; }
	const MoveValid* end()   const { return end_; }
	MoveValid operator[](int i) const { return begin_[i]; }
};


template<class Board>
class MoveIterator {  // Only returns valid moves.
protected:
	const Board& board_;
	MoveValid move_;
	i8 line_end_;

public:
	MoveIterator(const Board& b) : board_(b), move_(M_SWAP), line_end_(0) {
		if(board_.outcome() >= Outcome::DRAW){
			move_ = M_NONE;
		} else {
			++(*this); //find the first valid move
		}
	}
	MoveIterator(const Board& b, MoveValid m) : board_(b), move_(m) { }
	const MoveValid & operator * ()  const { return move_; }
	const MoveValid * operator -> () const { return & move_; }
	bool operator == (const MoveIterator & rhs) const { return (move_ == rhs.move_); }
	bool operator != (const MoveIterator & rhs) const { return (move_ != rhs.move_); }
	MoveIterator & operator ++ () {  //prefix form
		do {
			move_.x++;
			move_.xy++;

			if (move_.x >= line_end_) {
				move_.y++;
				if (move_.y >= board_.lines()) {  // done
					move_ = M_NONE;
					return *this;
				}
				move_.x = board_.line_start(move_.y);
				move_.xy = board_.xy(move_.x, move_.y);
				line_end_ = board_.line_end(move_.y);
			}
		} while (!board_.valid_move_fast(move_));

		return *this;
	}
};

/*
template<class Board>
class MoveIteratorUnique : public MoveIterator<Board> {
	HashSet hashes_;
public:
	MoveIteratorUnique(const Board & b) : MoveIterator<Board>(b) {
		hashes_.init(MoveIterator<Board>::board_.moves_avail());
		++(*this);
	}

	MoveIteratorUnique & operator ++ () { //prefix form
		while (true) {
			MoveIterator<Board>::operator++();
			if (MoveIterator<Board>::move_ == M_NONE) {
				return *this;
			}

			auto h = MoveIterator<Board>::board_.test_hash(MoveIterator<Board>::move_, MoveIterator<Board>::board_.to_play());
			if (hashes_.add(h)) {
				return *this;
			}
		}
	}
};


template<class Board, class Rand>
class MoveIteratorRandom : public MoveIterator<Board> {
	Rand & rand_;
	std::vector<MoveValid> moves_;

public:
	MoveIteratorRandom(const Board & b, Rand & r) : MoveIterator<Board>(b), rand_(r) {
		moves_.reserve(MoveIterator<Board>::board_.moves_avail());
		for(auto m : MoveIterator<Board>::board_) {
			moves_.push_back(m);
		}
		++(*this);
	}

	MoveIteratorRandom & operator ++ () { //prefix form
		auto len = moves_.size();
		if (len > 0) {
			// incremental fisher-yates shuffle. Choose a random element that hasn't yet been chosen.
			int i = rand_() % len;
			MoveIterator<Board>::move_ = moves_[i];
			moves_[i] = moves_[len - 1];
			moves_.pop_back();
		} else {
			MoveIterator<Board>::move_ = M_NONE;
		}
		return *this;
	}
};
*/

}; // namespace Morat
