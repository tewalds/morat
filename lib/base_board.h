
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <functional>
#include <ostream>
#include <string>
#include <vector>

#include "move.h"
#include "move_iterator.h"
#include "outcome.h"
#include "string.h"
#include "types.h"
#include "zobrist.h"


namespace Morat {

class BaseBoard {
public:

	static const int default_size = 8;
	static const int min_size = 3;
	static const int max_size = 16;
	static const int max_vecsize = max_size * max_size;

protected:
	i8      size_x_;
	i8      size_y_;
	Outcome outcome_;
	i8      win_type_;
	Side    to_play_;
	i16     moves_made_;
	Move    last_move_;  // used mainly for to_s

public:

	BaseBoard() {
		size_x_ = 0;
		size_y_ = 0;
		last_move_ = M_NONE;
		moves_made_ = 0;
		to_play_ = Side::P1;
		outcome_ = Outcome::UNKNOWN;
		win_type_ = 0;
	}

	virtual ~BaseBoard() {}


	// the game specific size representation of the board
	virtual int size() const { return size_max(); }

	// size of the grid, non-game specific
	i8 size_x()   const { return size_x_; }
	i8 size_y()   const { return size_y_; }
	i8 size_max() const { return std::max(size_x_, size_y_); }

	        i16 moves_made()   const { return moves_made_; }  // how many moves have been made so far?
	virtual i16 moves_remain() const { return (outcome_ >= Outcome::DRAW ? 0 : num_cells() - moves_made()); }  // max number of moves left to make
	virtual i16 moves_avail()  const { return moves_remain(); }  // how many are available to make right now?

	virtual int mem_size()  const = 0;
	virtual i16 vec_size()  const { return size_x_ * size_y_; }  // default to a square-ish board
	virtual i16 num_cells() const { return size_x_ * size_y_; }  // default to a square-ish board

	// Define the geometry of the board. Default assumes a square-ish board.
	// Can support any board without discontinuities and where the top and bottom are straight.
	virtual i8 line_start(i8 y) const { return 0; }
	virtual i8 line_end(i8 y)   const { return size_x_; }
	        i8 line_len(i8 y)   const { return line_end(y) - line_start(y); }

	// Given a move or coords, give a canonical offset for that move
	virtual i16 xy(i16 x, i16 y) const { return x + y * size_x_; }
	i16 xy(const Move & m)       const { return xy(m.x, m.y); }
	i16 xy(const MoveValid & m)  const { return m.xy; }

	// assumes valid x,y
	virtual Side get(i16 i)       const = 0;
	Side get(i8 x, i8 y)          const { return get(xy(x, y)); }
	Side get(const Move & m)      const { return get(xy(m)); }
	Side get(const MoveValid & m) const { return get(m.xy); }

	// Check that a move is on the board.
	bool onboard(i8 x, i8 y)           const { return (y >= 0 && y < size_y_ && x >= line_start(y) && x < line_end(y)); }
	bool onboard(const Move & m)       const { return onboard(m.x, m.y); }
	bool onboard(const MoveValid & m)  const { return m.onboard(); }

	// assumes x, y are in bounds and the game isn't already finished
	virtual bool valid_move_fast(i16 i)       const { return get(i) == Side::NONE; }
	bool valid_move_fast(i8 x, i8 y)          const { return valid_move_fast(xy(x, y)); }
	bool valid_move_fast(const Move & m)      const { return valid_move_fast(xy(m)); }
	bool valid_move_fast(const MoveValid & m) const { return valid_move_fast(m.xy); }
	//checks array bounds too
	bool valid_move(int x, int y)        const { return (outcome_ < Outcome::DRAW && onboard(x, y) && valid_move_fast(x, y)); }
	bool valid_move(const Move & m)      const { return (outcome_ < Outcome::DRAW && onboard(m)    && valid_move_fast(m)); }
	bool valid_move(const MoveValid & m) const { return (outcome_ < Outcome::DRAW && m.onboard()   && valid_move_fast(m)); }

	//iterators through neighbours of a position
	virtual const MoveValid * nb_begin(i16 i)       const = 0;
	const MoveValid * nb_begin(i8 x, i8 y)          const { return nb_begin(xy(x, y)); }
	const MoveValid * nb_begin(const Move & m)      const { return nb_begin(xy(m)); }
	const MoveValid * nb_begin(const MoveValid & m) const { return nb_begin(m.xy); }

	virtual const MoveValid * nb_end(i16 i)       const { return nb_end_small_hood(nb_begin(i)); }
	const MoveValid * nb_end(i8 x, i8 y)          const { return nb_end(xy(x, y)); }
	const MoveValid * nb_end(const Move & m)      const { return nb_end(xy(m)); }
	const MoveValid * nb_end(const MoveValid & m) const { return nb_end(m.xy); }

	virtual const MoveValid * nb_end_small_hood( const MoveValid * m) const = 0;
	virtual const MoveValid * nb_end_medium_hood(const MoveValid * m) const = 0;
	virtual const MoveValid * nb_end_large_hood( const MoveValid * m) const = 0;


	        std::string to_s(bool color) const;
	virtual std::string to_s(bool color, std::function<std::string(Move)> func) const = 0;
	friend std::ostream& operator<< (std::ostream &out, const BaseBoard & b) { return out << b.to_s(true); }

	Outcome outcome()  const { return outcome_; }
	i8      win_type() const { return win_type_; }

	Side to_play() const { return to_play_; }
	void to_play(Side s) { to_play_ = s; }

	// Iterate through the valid moves
	MoveIterator begin() const { return MoveIterator(*this); }
	MoveIterator end()   const { return MoveIterator(*this, M_NONE); }
	// TODO: unique and random iterator. Implement by returning a proxy object.

	virtual i16 knowledge() const { return 0; }

	virtual hash_t hash()     const = 0;
	std::string    hash_str() const;

	virtual void update_hash(const MoveValid & m, Side side) = 0;
	virtual hash_t test_hash(const MoveValid & m, Side side) const = 0;
	        hash_t test_hash(const MoveValid & m) const { return test_hash(m, to_play()); }

  bool move(const Move & m, bool checkwin = true, bool permanent = true) {
		return move(MoveValid(m, xy(m)), checkwin, permanent);
	}
	virtual bool move(const MoveValid & m, bool checkwin = true, bool permanent = true) = 0;

	//test if making this move would win, but don't actually make the move
	        Outcome test_outcome(const Move & m)                 const { return test_outcome(m, to_play()); }
	        Outcome test_outcome(const Move & m, Side turn)      const { return test_outcome(MoveValid(m, xy(m)), turn); }
	        Outcome test_outcome(const MoveValid & m)            const { return test_outcome(m, to_play()); }
	virtual Outcome test_outcome(const MoveValid & m, Side turn) const = 0;
};

}; // namespace Morat
