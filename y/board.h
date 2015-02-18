
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdio>
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
namespace Y {

/*
 * the board is represented as a flattened 2d array of the form:
 *   1 2 3
 * A 0 1 2     0 1 2     0 1 2
 * B 3 4 5 <=> 3 4   <=>  3 4
 * C 6 7 8     6           6
 */

/* neighbours are laid out in this pattern:
 *     12   6  13               12  6 13
 *   11   0   1   7          11  0  1  7
 * 17   5   X   2  14 <=> 17  5  X  2 14
 *   10   4   3   8       10  4  3  8
 *     16   9  15         16  9 15
 */
const MoveScore neighbours[18] = {
	MoveScore( 0,-1, 3), MoveScore(1,-1, 3), MoveScore(1, 0, 3), MoveScore( 0, 1, 3), MoveScore(-1, 1, 3), MoveScore(-1, 0, 3), //direct neighbours, clockwise
	MoveScore( 1,-2, 2), MoveScore(2,-1, 2), MoveScore(1, 1, 2), MoveScore(-1, 2, 2), MoveScore(-2, 1, 2), MoveScore(-1,-1, 2), //sides of ring 2, virtual connections
	MoveScore( 0,-2, 1), MoveScore(2,-2, 1), MoveScore(2, 0, 1), MoveScore( 0, 2, 1), MoveScore(-2, 2, 1), MoveScore(-2, 0, 1), //corners of ring 2, easy to block
	};

static MoveValid * staticneighbourlist[17] = {
	NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,
	NULL}; //one per boardsize


class Board{
public:

	static constexpr const char * name = "y";
	static const int default_size = 10;
	static const int min_size = 5;
	static const int max_size = 16;
	static const int max_vecsize = max_size * max_size;
	static const int num_win_types = 1;

	static const int pattern_cells = 18;
	typedef uint64_t Pattern;

	struct Cell {
		Side     piece;   //who controls this cell, 0 for none, 1,2 for players
		uint16_t size;    //size of this group of cells
mutable uint16_t parent;  //parent for this group of cells
		uint8_t  edge;    //which edges are this group connected to
		uint8_t  perm;    //is this a permanent piece or a randomly placed piece?
		Pattern  pattern; //the pattern of pieces for neighbours, but from their perspective. Rotate 180 for my perpective

		Cell() : piece(Side::NONE), size(0), parent(0), edge(0), perm(0), pattern(0) { }
		Cell(Side p, unsigned int a, unsigned int s, unsigned int e, Pattern t) :
			piece(p), size(s), parent(a), edge(e), perm(0), pattern(t) { }

		int numedges()   const { return BitsSetTable256[edge]; }

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
			if(board.outcome >= Outcome::DRAW){
				move = MoveValid(0, board.size, -1); //already done
			} else {
				if(unique)
					hashes.init(board.movesremain());
				++(*this); //find the first valid move
			}
		}

		const MoveValid & operator * ()  const { return move; }
		const MoveValid * operator -> () const { return & move; }
		bool done() const { return (move.y >= board.size); }
		bool operator == (const Board::MoveIterator & rhs) const { return (move == rhs.move); }
		bool operator != (const Board::MoveIterator & rhs) const { return (move != rhs.move); }
		MoveIterator & operator ++ (){ //prefix form
			while(true){
				do{
					move.x++;
					move.xy++;

					if(move.x >= lineend){
						move.y++;
						if(move.y >= board.size){ //done
							move.xy = -1;
							return *this;
						}
						move.x = board.linestart(move.y);
						move.xy = board.xy(move.x, move.y);
						lineend = board.lineend(move.y);
					}
				}while(!board.valid_move_fast(move));

				if(unique){
					uint64_t h = board.test_hash(move, board.toplay());
					if(!hashes.add(h))
						continue;
				}
				break;
			}
			return *this;
		}
	};

private:
	char size; //the length of one side of the hexagon
	char sizem1; //size - 1

	short num_cells;
	short nummoves;
	short unique_depth; //update and test rotations/symmetry with less than this many pieces on the board
	Move last;
	Side toPlay;
	Outcome outcome;

	std::vector<Cell> cells;
	Zobrist<6> hash;
	const MoveValid * neighbourlist;

public:
	Board(){
		size = 0;
	}

	Board(int s){
		size = s;
		sizem1 = s - 1;
		last = M_NONE;
		nummoves = 0;
		unique_depth = 5;
		toPlay = Side::P1;
		outcome = Outcome::UNKNOWN;
		neighbourlist = get_neighbour_list();
		num_cells = vecsize() - (size*sizem1/2);

		cells.resize(vecsize());

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
				cells[posxy] = Cell(s, posxy, 1, edges(x, y), pattern_reverse(p));
			}
		}
	}

/*	~Board(){
		printf("~Board");
	}
*/
	int memsize() const { return sizeof(Board) + sizeof(Cell)*vecsize(); }

	int get_size() const{ return size; }

	int vecsize() const { return size*size; }
	int numcells() const { return num_cells; }

	int num_moves() const { return nummoves; }
	int movesremain() const { return (won() >= Outcome::DRAW ? 0 : num_cells - nummoves); }

	int xy(int x, int y)   const { return   y*size +   x; }
	int xy(const Move & m) const { return m.y*size + m.x; }
	int xy(const MoveValid & m) const { return m.xy; }

	MoveValid yx(int i) const { return MoveValid(i % size, i / size, i); }

	int dist(const Move & a, const Move & b) const {
		return (abs(a.x - b.x) + abs(a.y - b.y) + abs((a.x + a.y) - (b.x + b.y)) )/2;
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

	int local(const Move & m, Side turn) const { return local(xy(m), turn); }
	int local(int i,          Side turn) const {
		Pattern p = pattern(i);
		Pattern x = ((p & 0xAAAAAAAAAull) >> 1) ^ (p & 0x555555555ull); // p1 is now when p1 or p2 but not both (ie off the board)
		p = x & (turn == Side::P1 ? p : p >> 1); // now just the selected player
		return (p & 0x000000FFF ? 3 : 0) |
		       (p & 0x000FFF000 ? 2 : 0) |
		       (p & 0xFFF000000 ? 1 : 0);
	}


	//assumes x, y are in array bounds
	bool onboard_fast(int x, int y)   const { return (  y +   x < size); }
	bool onboard_fast(const Move & m) const { return (m.y + m.x < size); }
	//checks array bounds too
	bool onboard(int x, int y)  const { return (  x >= 0 &&   y >= 0 && onboard_fast(x, y) ); }
	bool onboard(const Move & m)const { return (m.x >= 0 && m.y >= 0 && onboard_fast(m) ); }
	bool onboard(const MoveValid & m) const { return m.onboard(); }

	//assumes x, y are in bounds and the game isn't already finished
	bool valid_move_fast(int i)               const { return get(i) == Side::NONE; }
	bool valid_move_fast(int x, int y)        const { return valid_move_fast(xy(x, y)); }
	bool valid_move_fast(const Move & m)      const { return valid_move_fast(xy(m)); }
	bool valid_move_fast(const MoveValid & m) const { return valid_move_fast(m.xy); }
	//checks array bounds too
	bool valid_move(int x, int y)        const { return (outcome < Outcome::DRAW && onboard(x, y) && valid_move_fast(x, y)); }
	bool valid_move(const Move & m)      const { return (outcome < Outcome::DRAW && onboard(m)    && valid_move_fast(m)); }
	bool valid_move(const MoveValid & m) const { return (outcome < Outcome::DRAW && m.onboard()   && valid_move_fast(m)); }

	//iterator through neighbours of a position
	const MoveValid * nb_begin(int x, int y)   const { return nb_begin(xy(x, y)); }
	const MoveValid * nb_begin(const Move & m) const { return nb_begin(xy(m)); }
	const MoveValid * nb_begin(int i)          const { return &neighbourlist[i*18]; }

	const MoveValid * nb_end(int x, int y)   const { return nb_end(xy(x, y)); }
	const MoveValid * nb_end(const Move & m) const { return nb_end(xy(m)); }
	const MoveValid * nb_end(int i)          const { return nb_end(nb_begin(i)); }
	const MoveValid * nb_end(const MoveValid * m) const { return m + 6; }
	const MoveValid * nb_end_small_hood(const MoveValid * m) const { return m + 12; }
	const MoveValid * nb_end_big_hood(const MoveValid * m) const { return m + 18; }

	int edges(int x, int y) const;

	MoveValid * get_neighbour_list(){
		if(!staticneighbourlist[(int)size]){
			MoveValid * list = new MoveValid[vecsize()*18];
			MoveValid * a = list;
			for(int y = 0; y < size; y++){
				for(int x = 0; x < size; x++){
					Move pos(x,y);

					for(int i = 0; i < 18; i++){
						Move loc = pos + neighbours[i];
						*a = MoveValid(loc, (onboard(loc) ? xy(loc) : -1) );
						++a;
					}
				}
			}

			staticneighbourlist[(int)size] = list;
		}

		return staticneighbourlist[(int)size];
	}

	int linestart(int y) const { return 0; }
	int lineend(int y)   const { return (size - y); }
	int linelen(int y)   const { return lineend(y) - linestart(y); }

	std::string to_s(bool color) const;
	friend std::ostream& operator<< (std::ostream &out, const Board & b) { return out << b.to_s(true); }

	void print(bool color = true) const {
		printf("%s", to_s(color).c_str());
	}

	Outcome won() const {
		return outcome;
	}

	char getwintype() const { return outcome > Outcome::DRAW; }

	Side toplay() const {
		return toPlay;
	}

	MoveIterator moveit(bool unique = false) const {
		return MoveIterator(*this, (unique ? nummoves <= unique_depth : false));
	}

	void set(const Move & m, bool perm = true) {
		last = m;
		Cell * cell = & cells[xy(m)];
		cell->piece = toPlay;
		cell->perm = perm;
		nummoves++;
		update_hash(m, toPlay); //depends on nummoves
		toPlay = ~toPlay;
	}

	void unset(const Move & m) { //break win checks, but is a poor mans undo if all you care about is the hash
		toPlay = ~toPlay;
		update_hash(m, toPlay);
		nummoves--;
		Cell * cell = & cells[xy(m)];
		cell->piece = Side::NONE;
		cell->perm = 0;
	}

	int find_group(const MoveValid & m) const { return find_group(m.xy); }
	int find_group(const Move & m) const { return find_group(xy(m)); }
	int find_group(int x, int y)   const { return find_group(xy(x, y)); }
	int find_group(unsigned int i) const {
		unsigned int p = cells[i].parent;
		if(p != i){
			do{
				p = cells[p].parent;
			}while(p != cells[p].parent);
			cells[i].parent = p; //do path compression, but only the current one, not all, to avoid recursion
		}
		return p;
	}

	//join the groups of two positions, propagating group size, and edge connections
	//returns true if they're already the same group, false if they are now joined
	bool join_groups(const Move & a, const Move & b) { return join_groups(xy(a), xy(b)); }
	bool join_groups(int x1, int y1, int x2, int y2) { return join_groups(xy(x1, y1), xy(x2, y2)); }
	bool join_groups(int i, int j){
		i = find_group(i);
		j = find_group(j);

		if(i == j)
			return true;

		if(cells[i].size < cells[j].size) //force i's subtree to be bigger
			std::swap(i, j);

		cells[j].parent = i;
		cells[i].size   += cells[j].size;
		cells[i].edge   |= cells[j].edge;

		return false;
	}

	Cell test_cell(const Move & pos) const {
		Side turn = toplay();
		int posxy = xy(pos);

		Cell testcell = cells[find_group(pos)];
		for(const MoveValid * i = nb_begin(posxy), *e = nb_end(i); i < e; i++){
			if(i->onboard() && turn == get(i->xy)){
				const Cell * g = & cells[find_group(i->xy)];
				testcell.edge   |= g->edge;
				testcell.size   += g->size; //not quite accurate if it's joining the same group twice
				i++; //skip the next one
			}
		}
		return testcell;
	}

	int test_connectivity(const Move & pos) const {
		Cell testcell = test_cell(pos);
		return testcell.numedges();
	}

	int test_size(const Move & pos) const {
		Cell testcell = test_cell(pos);
		return testcell.size;
	}


	hash_t gethash() const {
		return (nummoves > unique_depth ? hash.get(0) : hash.get());
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
		if(nummoves > unique_depth){ //simple update, no rotations/symmetry
			hash.update(0, 3*xy(pos) + turn);
			return;
		}

		//mirror is simply flip x,y
		int x = pos.x,
		    y = pos.y,
		    z = sizem1 - x - y;

		hash.update(0,  3*xy(x, y) + turn);
		hash.update(1,  3*xy(z, y) + turn);
		hash.update(2,  3*xy(z, x) + turn);
		hash.update(3,  3*xy(x, z) + turn);
		hash.update(4,  3*xy(y, z) + turn);
		hash.update(5,  3*xy(y, x) + turn);
	}

	hash_t test_hash(const Move & pos) const {
		return test_hash(pos, toplay());
	}

	hash_t test_hash(const Move & pos, Side side) const {
		int turn = side.to_i();
		if(nummoves >= unique_depth) //simple test, no rotations/symmetry
			return hash.test(0, 3*xy(pos) + turn);

		int x = pos.x,
		    y = pos.y,
		    z = sizem1 - x - y;

		hash_t m = hash.test(0,  3*xy(x, y) + turn);
		m = std::min(m, hash.test(1,  3*xy(z, y) + turn));
		m = std::min(m, hash.test(2,  3*xy(z, x) + turn));
		m = std::min(m, hash.test(3,  3*xy(x, z) + turn));
		m = std::min(m, hash.test(4,  3*xy(y, z) + turn));
		m = std::min(m, hash.test(5,  3*xy(y, x) + turn));
		return m;
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
		return pattern(posxy) & ((1ull << 24) - 1);
	}

	Pattern pattern_small(const MoveValid & pos) const { return pattern_small(pos.xy); }
	Pattern pattern_small(const Move & pos)      const { return pattern_small(xy(pos)); }
	Pattern pattern_small(int posxy)             const {
		return pattern(posxy) & ((1ull << 12) - 1);
	}

	static Pattern pattern_reverse(Pattern p) { // switch perspectives (position out, or position in)
		return (((p & 0x03F03F03Full) << 6) | ((p & 0xFC0FC0FC0ull) >> 6));
	}

	static Pattern pattern_invert(Pattern p) { //switch players
		return ((p & 0xAAAAAAAAAull) >> 1) | ((p & 0x555555555ull) << 1);
	}
	static Pattern pattern_rotate(Pattern p) {
		return (((p & 0x003003003ull) << 10) | ((p & 0xFFCFFCFFCull) >> 2));
	}
	static Pattern pattern_mirror(Pattern p) {
		// HGFEDC BA9876 543210 -> DEFGHC 6789AB 123450
		return ((p & (3ull <<  6))      ) | ((p & (3ull <<  0))     ) | // 0,3 stay in place
		       ((p & (3ull << 10)) >>  8) | ((p & (3ull <<  2)) << 8) | // 1,5 swap
		       ((p & (3ull <<  8)) >>  4) | ((p & (3ull <<  4)) << 4) | // 2,4 swap
		       ((p & (3ull << 22)) >> 10) | ((p & (3ull << 12)) <<10) | // 6,B swap
		       ((p & (3ull << 20)) >>  6) | ((p & (3ull << 14)) << 6) | // 7,A swap
		       ((p & (3ull << 18)) >>  2) | ((p & (3ull << 16)) << 2) | // 8,9 swap
		       ((p & (3ull << 30))      ) | ((p & (3ull << 24))     ) | // F,C stay in place
		       ((p & (3ull << 34)) >>  8) | ((p & (3ull << 26)) << 8) | // H,D swap
		       ((p & (3ull << 32)) >>  4) | ((p & (3ull << 28)) << 4);  // G,E swap
	}
	static Pattern pattern_symmetry(Pattern p) { //takes a pattern and returns the representative version
		Pattern m = p;                 //012345
		m = std::min(m, (p = pattern_rotate(p)));//501234
		m = std::min(m, (p = pattern_rotate(p)));//450123
		m = std::min(m, (p = pattern_rotate(p)));//345012
		m = std::min(m, (p = pattern_rotate(p)));//234501
		m = std::min(m, (p = pattern_rotate(p)));//123450
		m = std::min(m, (p = pattern_mirror(pattern_rotate(p))));//012345 -> 054321
		m = std::min(m, (p = pattern_rotate(p)));//105432
		m = std::min(m, (p = pattern_rotate(p)));//210543
		m = std::min(m, (p = pattern_rotate(p)));//321054
		m = std::min(m, (p = pattern_rotate(p)));//432105
		m = std::min(m, (p = pattern_rotate(p)));//543210
		return m;
	}

	bool move(const Move & pos, bool checkwin = true, bool permanent = true) {
		return move(MoveValid(pos, xy(pos)), checkwin, permanent);
	}
	bool move(const MoveValid & pos, bool checkwin = true, bool permanent = true) {
		assert(outcome < Outcome::DRAW);

		if(!valid_move(pos))
			return false;

		Side turn = toplay();
		set(pos, permanent);

		// update the nearby patterns
		Pattern p = turn.to_i();
		for(const MoveValid * i = nb_begin(pos.xy), *e = nb_end_big_hood(i); i < e; i++){
			if(i->onboard()){
				cells[i->xy].pattern |= p;
			}
			p <<= 2;
		}

		// join the groups for win detection
		for(const MoveValid * i = nb_begin(pos.xy), *e = nb_end(i); i < e; i++){
			if(i->onboard() && turn == get(i->xy)){
				join_groups(pos.xy, i->xy);
				i++; //skip the next one. If it is the same group,
					 //it is already connected and forms a corner, which we can ignore
			}
		}

		// did I win?
		Cell * g = & cells[find_group(pos.xy)];
		if(g->numedges() == 3){
			outcome = +turn;
		}
		return true;
	}

	bool test_local(const Move & pos, Side turn) const { return test_local(MoveValid(pos, xy(pos)), turn); }
	bool test_local(const MoveValid & pos, Side turn) const {
		return (local(pos, turn) == 3);
	}

	//test if making this move would win, but don't actually make the move
	Outcome test_outcome(const Move & pos) const { return test_outcome(pos, toplay()); }
	Outcome test_outcome(const Move & pos, Side turn) const { return test_outcome(MoveValid(pos, xy(pos)), turn); }
	Outcome test_outcome(const MoveValid & pos) const { return test_outcome(pos, toplay()); }
	Outcome test_outcome(const MoveValid & pos, Side turn) const {
		if(test_local(pos, turn)){
			Cell testcell = cells[find_group(pos.xy)];
			int numgroups = 0;
			for(const MoveValid * i = nb_begin(pos), *e = nb_end(i); i < e; i++){
				if(i->onboard() && turn == get(i->xy)){
					const Cell * g = & cells[find_group(i->xy)];
					testcell.edge   |= g->edge;
					testcell.size   += g->size;
					i++; //skip the next one
					numgroups++;
				}
			}

			if(testcell.numedges() == 3)
				return +turn;
		}

		return Outcome::UNKNOWN;
	}
};

}; // namespace Y
}; // namespace Morat
