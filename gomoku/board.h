
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


class Board{
public:

	static constexpr const char * name = "gomoku";
	static const int default_size = 13;
	static const int min_size = 5;
	static const int max_size = 19;
	static const int max_vec_size = 19*19;
	static const int num_win_types = 1;

	static const int pattern_cells = 24;
	typedef uint64_t Pattern;

	struct Cell {
		Side    piece;   //who controls this cell, 0 for none, 1,2 for players
		uint8_t perm;    //is this a permanent piece or a randomly placed piece?
		Pattern pattern; //the pattern of pieces for neighbours, but from their perspective. Rotate 180 for my perpective

		Cell() : piece(Side::NONE), perm(0), pattern(0) { }
		Cell(Side p, Pattern t) : piece(p), perm(0), pattern(t) { }

		std::string to_s(int i) const;
	};

	class MoveIterator { //only returns valid moves...
		const Board & board;
		int lineend;
		MoveValid move;
		bool unique;
		HashSet hashes;
	public:
		MoveIterator(const Board & b, bool Unique) : board(b), lineend(0), move(Move(M_SWAP), -1), unique(Unique) {
			if(board.outcome_ >= Outcome::DRAW){
				move = MoveValid(0, board.get_size(), -1); //already done
			} else {
				if(unique)
					hashes.init(board.moves_remain());
				++(*this); //find the first valid move
			}
		}

		const MoveValid & operator * ()  const { return move; }
		const MoveValid * operator -> () const { return & move; }
		bool done() const { return (move.y >= board.get_size()); }
		bool operator == (const Board::MoveIterator & rhs) const { return (move == rhs.move); }
		bool operator != (const Board::MoveIterator & rhs) const { return (move != rhs.move); }
		MoveIterator & operator ++ (){ //prefix form
			while(true){
				do{
					move.x++;
					move.xy++;

					if(move.x >= lineend){
						move.y++;
						if(move.y >= board.get_size()){ //done
							move.xy = -1;
							return *this;
						}
						move.x = board.linestart(move.y);
						move.xy = board.xy(move.x, move.y);
						lineend = board.lineend(move.y);
					}
				}while(!board.valid_move_fast(move));

				if(unique){
					uint64_t h = board.test_hash(move, board.to_play());
					if(!hashes.add(h))
						continue;
				}
				break;
			}
			return *this;
		}
	};

private:
	char size; //the length of one side
	char sizem1; //size - 1

	short num_cells_;
	short num_moves_;
	Move last_move_;
	Side to_play_;
	Outcome outcome_;
	char win_type_; //0 no win, 1 = 5 in a row

	std::vector<Cell> cells;
	Zobrist<1> hash;
	std::shared_ptr<MoveValid> neighbourlist;

public:
	Board(){
		size = 0;
	}

	Board(int s){
		size = s;
		sizem1 = s - 1;
		last_move_ = M_NONE;
		num_moves_ = 0;
		to_play_ = Side::P1;
		outcome_ = Outcome::UNKNOWN;
		win_type_ = 0;
		neighbourlist = get_neighbour_list();
		num_cells_ = vec_size();

		cells.resize(vec_size());

		for(int y = 0; y < size; y++){
			for(int x = 0; x < size; x++){
				int posxy = xy(x, y);
				Pattern p = 0, j = 3;
				for(const MoveValid * i = nb_begin(posxy), *e = nb_end_big_hood(i); i < e; i++){
					if(!i->onboard())
						p |= j;
					j <<= 2;
				}
				Side s = (onboard(x, y) ? Side::NONE : Side::UNDEF);
				cells[posxy] = Cell(s, pattern_reverse(p));
			}
		}
	}

/*	~Board(){
		printf("~Board");
	}
*/

	int get_size() const{ return size; }

	int mem_size() const { return sizeof(Board) + sizeof(Cell)*vec_size(); }
	int vec_size() const { return size*size; }
	int num_cells() const { return num_cells_; }

	int moves_made() const { return num_moves_; }
	int moves_remain() const { return (outcome() >= Outcome::DRAW ? 0 : num_cells_ - num_moves_); }

	int xy(int x, int y)   const { return   y*size +   x; }
	int xy(const Move & m) const { return m.y*size + m.x; }
	int xy(const MoveValid & m) const { return m.xy; }

	MoveValid yx(int i) const { return MoveValid(i % size, i / size, i); }

	int dist(const Move & a, const Move & b) const {
		return std::max(abs(a.x - b.x), abs(a.y - b.y));
	}

	const Cell * cell(int i)          const { return & cells[i]; }
	const Cell * cell(int x, int y)   const { return cell(xy(x,y)); }
	const Cell * cell(const Move & m) const { return cell(xy(m)); }
	const Cell * cell(const MoveValid & m) const { return cell(m.xy); }

	//assumes valid x,y
	Side get(int i)          const { return cells[i].piece; }
	Side get(int x, int y)   const { return get(xy(x, y)); }
	Side get(const Move & m) const { return get(xy(m)); }
	Side get(const MoveValid & m) const { return get(m.xy); }

	Side geton(const MoveValid & m) const { return (m.onboard() ? get(m.xy) : Side::UNDEF); }

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


	//assumes x, y are in array bounds
	bool onboard_fast(int x, int y)   const { return true; }
	bool onboard_fast(const Move & m) const { return true; }
	//checks array bounds too
	bool onboard(int x, int y)  const { return (  x >= 0 &&   y >= 0 &&   x < size &&   y < size && onboard_fast(x, y) ); }
	bool onboard(const Move & m)const { return (m.x >= 0 && m.y >= 0 && m.x < size && m.y < size && onboard_fast(m) ); }
	bool onboard(const MoveValid & m) const { return m.onboard(); }

	//assumes x, y are in bounds and the game isn't already finished
	bool valid_move_fast(int i)               const { return get(i) == Side::NONE; }
	bool valid_move_fast(int x, int y)        const { return valid_move_fast(xy(x, y)); }
	bool valid_move_fast(const Move & m)      const { return valid_move_fast(xy(m)); }
	bool valid_move_fast(const MoveValid & m) const { return valid_move_fast(m.xy); }
	//checks array bounds too
	bool valid_move(int x, int y)        const { return (outcome_ < Outcome::DRAW && onboard(x, y) && valid_move_fast(x, y)); }
	bool valid_move(const Move & m)      const { return (outcome_ < Outcome::DRAW && onboard(m)    && valid_move_fast(m)); }
	bool valid_move(const MoveValid & m) const { return (outcome_ < Outcome::DRAW && m.onboard()   && valid_move_fast(m)); }

	//iterator through neighbours of a position
	const MoveValid * nb_begin(int x, int y)   const { return nb_begin(xy(x, y)); }
	const MoveValid * nb_begin(const Move & m) const { return nb_begin(xy(m)); }
	const MoveValid * nb_begin(int i)          const { return neighbourlist.get() + i*24; }

	const MoveValid * nb_end(int x, int y)   const { return nb_end(xy(x, y)); }
	const MoveValid * nb_end(const Move & m) const { return nb_end(xy(m)); }
	const MoveValid * nb_end(int i)          const { return nb_end(nb_begin(i)); }
	const MoveValid * nb_end(const MoveValid * m) const { return m + 8; }
	const MoveValid * nb_end_small_hood(const MoveValid * m) const { return m + 16; }
	const MoveValid * nb_end_big_hood(const MoveValid * m) const { return m + 24; }

	std::shared_ptr<MoveValid> get_neighbour_list() {
		std::shared_ptr<MoveValid> list(new MoveValid[vec_size()*24]);
		MoveValid * a = list.get();
		for(int y = 0; y < size; y++){
			for(int x = 0; x < size; x++){
				Move pos(x,y);

				for(int i = 0; i < 24; i++){
					Move loc = pos + neighbours[i];
					*a = MoveValid(loc, (onboard(loc) ? xy(loc) : -1) );
					++a;
				}
			}
		}

		return list;
	}

	int linestart(int y) const { return 0; }
	int lineend(int y)   const { return size; }
	int linelen(int y)   const { return size; }

	std::string to_s(bool color) const;
	std::string to_s(bool color, std::function<std::string(Move)> func) const;

	friend std::ostream& operator<< (std::ostream &out, const Board & b) { return out << b.to_s(true); }

	void print(bool color = true) const {
		printf("%s", to_s(color).c_str());
	}

	Outcome outcome() const {
		return outcome_;
	}

	char win_type() const { return win_type_; }

	Side to_play() const {
		return to_play_;
	}

	MoveIterator moveit(bool unique = false) const {
		return MoveIterator(*this, false);
	}

	void set(const Move & m, bool perm = true) {
		last_move_ = m;
		Cell * cell = & cells[xy(m)];
		cell->piece = to_play_;
		cell->perm = perm;
		num_moves_++;
		to_play_ = ~to_play_;
	}

	void unset(const Move & m) { //break win checks, but is a poor mans undo if all you care about is the hash
		to_play_ = ~to_play_;
		num_moves_--;
		Cell * cell = & cells[xy(m)];
		cell->piece = Side::NONE;
		cell->perm = 0;
	}

	hash_t gethash() const {
		return hash.get(0);
	}

	std::string hashstr() const {
		static const char hexlookup[] = "0123456789abcdef";
		char buf[19] = "0x";
		hash_t val = gethash();
		for(int i = 15; i >= 0; i--){
			buf[i+2] = hexlookup[val & 15];
			val >>= 4;
		}
		buf[18] = '\0';
		return (char *)buf;
	}

	void update_hash(const Move & pos, Side side) {
		int turn = side.to_i();
		hash.update(0, 3*xy(pos) + turn);
		return;
	}

	hash_t test_hash(const Move & pos) const {
		return test_hash(pos, to_play());
	}

	hash_t test_hash(const Move & pos, Side side) const {
		int turn = side.to_i();
		return hash.test(0, 3*xy(pos) + turn);
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

	bool move(const Move & pos, bool checkwin = true, bool permanent = true) {
		return move(MoveValid(pos, xy(pos)), checkwin, permanent);
	}
	bool move(const MoveValid & pos, bool checkwin = true, bool permanent = true) {
		assert(outcome_ < Outcome::DRAW);

		if(!valid_move(pos))
			return false;

		Side turn = to_play();

		if(checkwin) {
			outcome_ = test_outcome(pos, turn);
			if(outcome_ > Outcome::DRAW) {
				win_type_ = 1;
			}
		}

		set(pos, permanent);

		// update the nearby patterns
		Pattern p = turn.to_i();
		for(const MoveValid * i = nb_begin(pos.xy), *e = nb_end_big_hood(i); i < e; i++){
			if(i->onboard()){
				cells[i->xy].pattern |= p;
			}
			p <<= 2;
		}

		return true;
	}

	bool test_local(const Move & pos, Side turn) const { return test_local(MoveValid(pos, xy(pos)), turn); }
	bool test_local(const MoveValid & pos, Side turn) const {
		return (local(pos, turn) == 3);
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
					nb = nb_begin(nb)[d];
					if (nb.onboard() && get(nb) == turn)
						num++;
					else
						break;
				}
				nb = pos;
				for (int i = 0; i < 4; i++) {
					nb = nb_begin(nb)[d + 4];
					if (nb.onboard() && get(nb) == turn)
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
};

}; // namespace Gomoku
}; // namespace Morat
