
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <functional>
#include <ostream>
#include <string>
#include <vector>

#include "../lib/base_board.h"
#include "../lib/bitcount.h"
#include "../lib/hashset.h"
#include "../lib/move.h"
#include "../lib/outcome.h"
#include "../lib/string.h"
#include "../lib/types.h"
#include "../lib/zobrist.h"


namespace Morat {
namespace Gomoku {

/*
 * the board is represented as a flattened 2d array of the form:
 *   1 2 3
 * A 0 1 2
 * B 3 4 5
 * C 6 7 8
 *
 * neighbours are laid out in this pattern:
 *  8  16 9 17 10
 * 23  0  1  2 18
 * 15  7  X  3 11
 * 22  6  5  4 19
 * 14 21 13 20 12
 */

const MoveScore neighbours[24] = {
	MoveScore(-1,-1, 3), MoveScore(0,-1, 3), MoveScore(1,-1, 3), MoveScore(1, 0, 3), MoveScore(1, 1, 3), MoveScore( 0, 1, 3), MoveScore(-1, 1, 3), MoveScore(-1, 0, 3), //direct neighbours, clockwise
	MoveScore(-2,-2, 2), MoveScore(0,-2, 2), MoveScore(2,-2, 2), MoveScore(2, 0, 2), MoveScore(2, 2, 2), MoveScore( 0, 2, 2), MoveScore(-2, 2, 2), MoveScore(-2, 0, 2), //corners
	MoveScore(-1,-2, 1), MoveScore(1,-2, 1), MoveScore(2,-1, 1), MoveScore(2, 1, 1), MoveScore(1, 2, 1), MoveScore(-1, 2, 1), MoveScore(-2, 1, 1), MoveScore(-2,-1, 1), //knight's move
};

static MoveValid * staticneighbourlist[20] = {
	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL,
}; //one per boardsize


class Board : public BaseBoard {
public:

	static constexpr const char * name = "gomoku";
	static const int default_size = 13;
	static const int min_size = 5;
	static const int max_size = 19;
	static const int max_vecsize = 19*19;
	static const int num_win_types = 1;

	static const int pattern_cells = 24;

	struct Cell {
		Side    piece;   //who controls this cell, 0 for none, 1,2 for players
		uint8_t perm;    //is this a permanent piece or a randomly placed piece?
		Pattern pattern; //the pattern of pieces for neighbours, but from their perspective. Rotate 180 for my perpective

		Cell() : piece(Side::NONE), perm(0), pattern(0) { }
		Cell(Side p, Pattern t) : piece(p), perm(0), pattern(t) { }

		std::string to_s(int i) const;
	};


private:

	std::vector<Cell> cells;
	Zobrist<1> hash_;
	const MoveValid * neighbourlist;

public:
	Board() : BaseBoard() {	}

	Board(int s) : BaseBoard() {
		size_x_ = s;
		size_y_ = s;
		neighbourlist = get_neighbour_list();

		cells.resize(vec_size());

		for(int y = 0; y < size_y_; y++){
			for(int x = 0; x < size_x_; x++){
				int posxy = xy(x, y);
				Pattern p = 0, j = 3;
				for(const MoveValid * i = nb_begin(posxy), *e = nb_end_large_hood(i); i < e; i++){
					if(!i->onboard())
						p |= j;
					j <<= 2;
				}
				Side s = (onboard(x, y) ? Side::NONE : Side::UNDEF);
				cells[posxy] = Cell(s, pattern_reverse(p));
			}
		}
	}

	~Board() {}

	int mem_size() const { return sizeof(Board) + sizeof(Cell) * vec_size(); }

	int dist(const Move & a, const Move & b) const {
		return std::max(abs(a.x - b.x), abs(a.y - b.y));
	}

	const Cell * cell(int i)          const { return & cells[i]; }
	const Cell * cell(int x, int y)   const { return cell(xy(x,y)); }
	const Cell * cell(const Move & m) const { return cell(xy(m)); }
	const Cell * cell(const MoveValid & m) const { return cell(m.xy); }

	//assumes valid x,y
	using BaseBoard::get;
	Side get(i16 i)          const { return cells[i].piece; }


	int local(const MoveValid & m, Side turn) const { return local(m.xy, turn); }
	int local(const Move      & m, Side turn) const { return local(xy(m), turn); }
	int local(int i,               Side turn) const {
		Pattern p = pattern(i);
		Pattern x = ((p & 0xAAAAAAAAAAAAull) >> 1) ^ (p & 0x555555555555ull); // p1 is now when p1 or p2 but not both (ie off the board)
		p = x & (turn == Side::P1 ? p : p >> 1); // now just the selected player
		return (p & 0x00000000FFFF ? 3 : 0) |
		       (p & 0x0000FFFF0000 ? 2 : 0) |
		       (p & 0xFFFF00000000 ? 1 : 0);
	}

	bool test_local(const MoveValid & pos, Side turn) const {
		return (local(pos, turn) == 3);
	}

	//iterator through neighbours of a position
	using BaseBoard::nb_begin;
	const MoveValid * nb_begin(i16 i) const { return &neighbourlist[i*24]; }
	using BaseBoard::nb_end;
	const MoveValid * nb_end(i16 i)   const { return nb_end_small_hood(nb_begin(i)); }
	const MoveValid * nb_end_small_hood( const MoveValid * m) const { return m + 8; }
	const MoveValid * nb_end_medium_hood(const MoveValid * m) const { return m + 16; }
	const MoveValid * nb_end_large_hood( const MoveValid * m) const { return m + 24; }

	MoveValid * get_neighbour_list() {
		if(!staticneighbourlist[(int)size_x_]){
			MoveValid * list = new MoveValid[vec_size()*24];
			MoveValid * a = list;
			for(int y = 0; y < size_y_; y++){
				for(int x = 0; x < size_x_; x++){
					Move pos(x, y);

					for(int i = 0; i < 24; i++){
						Move loc = pos + neighbours[i];
						*a = MoveValid(loc, (onboard(loc) ? xy(loc) : -1) );
						++a;
					}
				}
			}

			staticneighbourlist[(int)size_x_] = list;
		}

		return staticneighbourlist[(int)size_x_];
	}

	using BaseBoard::to_s;
	std::string to_s(bool color, std::function<std::string(Move)> func) const;

	void set(const Move & m, bool perm = true) {
		last_move_ = m;
		Cell * cell = & cells[xy(m)];
		cell->piece = to_play_;
		cell->perm = perm;
		moves_made_++;
		to_play_ = ~to_play_;
	}

	void unset(const Move & m) { //break win checks, but is a poor mans undo if all you care about is the hash
		to_play_ = ~to_play_;
		moves_made_--;
		Cell * cell = & cells[xy(m)];
		cell->piece = Side::NONE;
		cell->perm = 0;
	}

	hash_t hash() const {
		return hash_.get(0);
	}

	void update_hash(const MoveValid & m, Side side) {
		hash_.update(0, 3 * m.xy + side.to_i());
	}

	hash_t test_hash(const MoveValid & m, Side side) const {
		return hash_.test(0, 3 * m.xy + side.to_i());
	}

	Pattern sympattern(const MoveValid & pos) const { return sympattern(pos.xy); }
	Pattern sympattern(const Move & pos)      const { return sympattern(xy(pos)); }
	Pattern sympattern(int posxy)             const { return pattern_symmetry(pattern(posxy)); }

	Pattern pattern(const MoveValid & pos) const { return pattern(pos.xy); }
	Pattern pattern(const Move & pos)      const { return pattern(xy(pos)); }
	Pattern pattern(int posxy)             const {
		// this is from the opposite perspective
		// so rotate into this move's perspective
		return pattern_reverse(cells[posxy].pattern);
	}

	Pattern pattern_medium(const MoveValid & pos) const { return pattern_medium(pos.xy); }
	Pattern pattern_medium(const Move & pos)      const { return pattern_medium(xy(pos)); }
	Pattern pattern_medium(int posxy)             const {
		return pattern(posxy) & ((1ull << 32) - 1);
	}

	Pattern pattern_small(const MoveValid & pos) const { return pattern_small(pos.xy); }
	Pattern pattern_small(const Move & pos)      const { return pattern_small(xy(pos)); }
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

	i16 knowledge() const {
		return 0;
	}

	using BaseBoard::move;
	bool move(const MoveValid & m, bool checkwin = true, bool permanent = true) {
		assert(outcome_ < Outcome::DRAW);

		if(!valid_move(m))
			return false;

		Side turn = to_play();

		if(checkwin) {
			outcome_ = test_outcome(m, turn);
			if(outcome_ > Outcome::DRAW) {
				win_type_ = 1;
			}
		}

		set(m, permanent);

		// update the nearby patterns
		Pattern p = turn.to_i();
		for(const MoveValid * i = nb_begin(m.xy), *e = nb_end_large_hood(i); i < e; i++){
			if(i->onboard()){
				cells[i->xy].pattern |= p;
			}
			p <<= 2;
		}

		return true;
	}

	//test if making this move would win, but don't actually make the move
	using BaseBoard::test_outcome;
	Outcome test_outcome(const MoveValid & m, Side turn) const {
		if (test_local(m, turn)) {
			for (int d = 0; d < 4; d++) {
				int num = 1;
				MoveValid nb = m;
				for (int i = 0; i < 4; i++) {
					nb = BaseBoard::nb_begin(nb)[d];
					if (nb.onboard() && BaseBoard::get(nb) == turn)
						num++;
					else
						break;
				}
				nb = m;
				for (int i = 0; i < 4; i++) {
					nb = BaseBoard::nb_begin(nb)[d + 4];
					if (nb.onboard() && BaseBoard::get(nb) == turn)
						num++;
					else
						break;
				}
				if (num >= 5)
					return +turn;
			}
		}

		if(moves_made_+1 == num_cells())
			return Outcome::DRAW;

		return Outcome::UNKNOWN;
	}
};

}; // namespace Gomoku
}; // namespace Morat
