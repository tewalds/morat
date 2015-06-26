
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "../lib/bitcount.h"
#include "../lib/board_grid_oct.h"
#include "../lib/hashset.h"
#include "../lib/move.h"
#include "../lib/outcome.h"
#include "../lib/string.h"
#include "../lib/types.h"
#include "../lib/zobrist.h"


namespace Morat {
namespace Gomoku {

class Board : public BoardGridOct<Board> {
public:

	static constexpr const char * name = "gomoku";
	static constexpr const char * default_size = "13";
	static const int min_size = 5;
	static const int max_size = 19;
	static const int max_vec_size = max_size * max_size;

	static const int num_win_types = 1;
	static const char* win_names[num_win_types];

	static const int pattern_cells = 24;
	typedef uint64_t Pattern;

	struct Cell {
		Side    piece;   //who controls this cell, 0 for none, 1,2 for players
		uint8_t perm;    //is this a permanent piece or a randomly placed piece?
		Pattern pattern; //the pattern of pieces for neighbors, but from their perspective. Rotate 180 for my perpective

		Cell() : piece(Side::NONE), perm(0), pattern(0) { }
		Cell(Side p, Pattern t) : piece(p), perm(0), pattern(t) { }

		std::string to_s(int i) const;
	};

private:
	int8_t size_;
	short num_cells_;
	short num_moves_;
	Move last_move_;
	Side to_play_;
	Outcome outcome_;

	std::vector<Cell> cells_;
	Zobrist<1> hash;
	std::shared_ptr<MoveValid> neighbor_list_;

public:
	Board() = delete;
	explicit Board(std::string s) {
		assert(size(s));
	}

	bool size(std::string s) {
		if (!valid_size(s))
			return false;
		size_ = from_str<int>(s);
		neighbor_list_ = gen_neighbor_list();
		num_cells_ = vec_size();
		cells_.resize(vec_size());
		clear();
		return true;
	}

	void clear() {
		last_move_ = M_NONE;
		num_moves_ = 0;
		to_play_ = Side::P1;
		outcome_ = Outcome::UNKNOWN;
		hash.clear();

		for(int y = 0; y < size_; y++){
			for(int x = 0; x < size_; x++){
				MoveValid pos = move_valid(x, y);
				Side s = (on_board(x, y) ? Side::NONE : Side::UNDEF);
				cells_[pos.xy] = Cell(s, init_pattern(pos));
			}
		}
	}

	std::string size() const {
		return to_str<int>(size_);
	}

	static bool valid_size(const std::string& s) {
		int size = from_str<int>(s);
		return (min_size <= size && size <= max_size);
	}

	int mem_size() const { return sizeof(Board) + sizeof(Cell)*vec_size(); }
	int vec_size() const { return size_*size_; }
	int num_cells() const { return num_cells_; }

	int moves_made() const { return num_moves_; }
	int moves_avail() const { return (outcome() >= Outcome::DRAW ? 0 : num_cells_ - num_moves_); }
	int moves_remain() const { return moves_avail(); }

	int xy(int x, int y)   const { return   y*size_ +   x; }
	int xy(const Move & m) const { return m.y*size_ + m.x; }
	int xy(const MoveValid & m) const { return m.xy; }

	MoveValid yx(int i) const { return MoveValid(i % size_, i / size_, i); }

	int dist(const Move & a, const Move & b) const {
		return std::max(abs(a.x - b.x), abs(a.y - b.y));
	}

	const Cell * cell(int i)          const { return & cells_[i]; }
	const Cell * cell(int x, int y)   const { return cell(xy(x,y)); }
	const Cell * cell(const Move & m) const { return cell(xy(m)); }
	const Cell * cell(const MoveValid & m) const { return cell(m.xy); }

	//assumes valid x,y
	Side get(int i)          const { return cells_[i].piece; }
	Side get(int x, int y)   const { return get(xy(x, y)); }
	Side get(const Move & m) const { return get(xy(m)); }
	Side get(const MoveValid & m) const { return get(m.xy); }

	//assumes x, y are in array bounds
	bool on_board_fast(int x, int y)   const { return true; }
	bool on_board_fast(const Move & m) const { return true; }
	//checks array bounds too
	bool on_board(int x, int y)  const { return (  x >= 0 &&   y >= 0 &&   x < size_ &&   y < size_ && on_board_fast(x, y) ); }
	bool on_board(const Move & m)const { return (m.x >= 0 && m.y >= 0 && m.x < size_ && m.y < size_ && on_board_fast(m) ); }
	bool on_board(const MoveValid & m) const { return m.on_board(); }

	//assumes x, y are in bounds and the game isn't already finished
	bool valid_move_fast(int i)               const { return get(i) == Side::NONE; }
	bool valid_move_fast(int x, int y)        const { return valid_move_fast(xy(x, y)); }
	bool valid_move_fast(const Move & m)      const { return valid_move_fast(xy(m)); }
	bool valid_move_fast(const MoveValid & m) const { return valid_move_fast(m.xy); }
	//checks array bounds too
	bool valid_move(int x, int y)        const { return (outcome_ < Outcome::DRAW && on_board(x, y) && valid_move_fast(x, y)); }
	bool valid_move(const Move & m)      const { return (outcome_ < Outcome::DRAW && on_board(m)    && valid_move_fast(m)); }
	bool valid_move(const MoveValid & m) const { return (outcome_ < Outcome::DRAW && m.on_board()   && valid_move_fast(m)); }

	int lines()           const { return size_; }
	int line_start(int y) const { return 0; }
	int line_end(int y)   const { return size_; }
	int line_len(int y)   const { return line_end(y) - line_start(y); }

	std::string to_s(bool color) const;
	std::string to_s(bool color, std::function<std::string(Move)> func) const;

	friend std::ostream& operator<< (std::ostream &out, const Board & b) { return out << b.to_s(true); }

	void print(bool color = true) const {
		printf("%s", to_s(color).c_str());
	}

	Outcome outcome() const {
		return outcome_;
	}

	int8_t win_type() const { return 0; }

	Side to_play() const {
		return to_play_;
	}

	hash_t gethash() const {
		return hash.get(0);
	}

	void update_hash(const MoveValid & pos, Side side) {
		int turn = side.to_i();
		hash.update(0, 3 * pos.xy + turn);
	}

	hash_t test_hash(const MoveValid & pos) const {
		return test_hash(pos, to_play());
	}

	hash_t test_hash(const MoveValid & pos, Side side) const {
		int turn = side.to_i();
		return hash.test(0, 3 * pos.xy + turn);
	}

	bool move(const Move & pos, bool checkwin = true, bool permanent = true) {
		return move(MoveValid(pos, xy(pos)), checkwin, permanent);
	}
	bool move(const MoveValid & pos, bool checkwin = true, bool permanent = true) {
		assert(!outcome_.solved());

		if(!valid_move(pos))
			return false;

		if(checkwin) {
			outcome_ = test_outcome(pos, to_play_);
		}

		last_move_ = pos;
		num_moves_++;

		Cell& cell = cells_[pos.xy];
		cell.piece = to_play_;
		cell.perm = permanent;

		update_hash(pos, to_play_); //depends on num_moves_
		update_pattern(pos, to_play_);

		to_play_ = ~to_play_;

		return true;
	}

	//test if making this move would win, but don't actually make the move
	Outcome test_outcome(const Move & pos) const { return test_outcome(pos, to_play()); }
	Outcome test_outcome(const Move & pos, Side turn) const { return test_outcome(MoveValid(pos, xy(pos)), turn); }
	Outcome test_outcome(const MoveValid & pos) const { return test_outcome(pos, to_play()); }
	Outcome test_outcome(const MoveValid & pos, Side turn) const {
		if (test_local(pos, turn)) {
			for (int d = 0; d < 4; d++) {
				int num = 1;
				MoveValid nb = pos;
				for (int i = 0; i < 4; i++) {
					nb = neighbors(nb)[d];
					if (nb.on_board() && get(nb) == turn)
						num++;
					else
						break;
				}
				nb = pos;
				for (int i = 0; i < 4; i++) {
					nb = neighbors(nb)[d + 4];
					if (nb.on_board() && get(nb) == turn)
						num++;
					else
						break;
				}
				if (num >= 5)
					return +turn;
			}
		}

		if(num_moves_+1 == num_cells_)
			return Outcome::DRAW;

		return Outcome::UNKNOWN;
	}

private:
	std::shared_ptr<MoveValid> gen_neighbor_list() const;

	friend class BoardGridOct;
	friend class BoardBase;
};

}; // namespace Gomoku
}; // namespace Morat
