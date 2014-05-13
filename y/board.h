
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#include "../lib/hashset.h"
#include "../lib/string.h"
#include "../lib/types.h"
#include "../lib/zobrist.h"

#include "move.h"

using namespace std;

static const int BitsSetTable64[] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6
};

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

	static const int default_size = 10;
	static const int min_size = 5;
	static const int max_size = 16;

	static const int pattern_cells = 18;
	typedef uint64_t Pattern;

	struct Cell {
		uint8_t piece;  //who controls this cell, 0 for none, 1,2 for players
		uint8_t size;   //size of this group of cells
mutable uint8_t parent; //parent for this group of cells. 8 bits limits board size to 16 until it's no longer stored as a square
		unsigned edge : 3;  //which edges are this group connected to
		unsigned perm : 1;  //is this a permanent piece or a randomly placed piece?
		Pattern  pattern: 36; //the pattern of pieces for neighbours, but from their perspective. Rotate 180 for my perpective

		Cell() : piece(73), size(0), parent(0), edge(0), perm(0), pattern(0) { }
		Cell(unsigned int p, unsigned int a, unsigned int s, unsigned int e, Pattern t) :
			piece(p), size(s), parent(a), edge(e), perm(0), pattern(t) { }

		int numedges()   const { return BitsSetTable64[(int)edge]; }

		string to_s(int i) const {
			return "Cell " + to_str(i) +": "
				"piece: " + to_str((int)piece)+
				", size: " + to_str((int)size) +
				", parent: " + to_str((int)parent) +
				", edge: " + to_str((int)edge) + "/" + to_str(numedges()) +
				", perm: " + to_str((int)perm) +
				", pattern: " + to_str((int)pattern);
		}
	};

	class MoveIterator { //only returns valid moves...
		const Board & board;
		int lineend;
		MoveValid move;
		bool unique;
		HashSet hashes;
	public:
		MoveIterator(const Board & b, bool Unique) : board(b), lineend(0), move(Move(M_SWAP), -1), unique(Unique) {
			if(board.outcome >= 0){
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

						move.x = 0;
						move.xy = move.y * board.size;
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
	char toPlay;
	char outcome; //-3 = unknown, 0 = tie, 1,2 = player win

	vector<Cell> cells;
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
		toPlay = 1;
		outcome = -3;
		neighbourlist = get_neighbour_list();
		num_cells = vecsize() - (size*sizem1/2);

		cells.resize(vecsize());

		for(int y = 0; y < size; y++){
			for(int x = 0; x < lineend(y); x++){
				int posxy = xy(x, y);
				Pattern p = 0, j = 3;
				for(const MoveValid * i = nb_begin(posxy), *e = nb_end_big_hood(i); i < e; i++){
					if(!i->onboard())
						p |= j;
					j <<= 2;
				}
				cells[posxy] = Cell(0, posxy, 1, edges(x, y), pattern_reverse(p));
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
	int movesremain() const { return (won() >= 0 ? 0 : num_cells - nummoves); }

	int xy(int x, int y)   const { return   y*size +   x; }
	int xy(const Move & m) const { return m.y*size + m.x; }
	int xy(const MoveValid & m) const { return m.xy; }

	MoveValid yx(int i) const { return MoveValid(i % size, i / size, i); }

	const Cell * cell(int i)          const { return & cells[i]; }
	const Cell * cell(int x, int y)   const { return cell(xy(x,y)); }
	const Cell * cell(const Move & m) const { return cell(xy(m)); }
	const Cell * cell(const MoveValid & m) const { return cell(m.xy); }


	//assumes valid x,y
	int get(int i)          const { return cells[i].piece; }
	int get(int x, int y)   const { return get(xy(x, y)); }
	int get(const Move & m) const { return get(xy(m)); }
	int get(const MoveValid & m) const { return get(m.xy); }

	int geton(const MoveValid & m) const { return (m.onboard() ? get(m.xy) : 0); }

	int local(const Move & m, char turn) const { return local(xy(m), turn); }
	int local(int i,          char turn) const {
		Pattern p = pattern(i);
		Pattern x = ((p & 0xAAAAAAAAAull) >> 1) ^ (p & 0x555555555ull); // p1 is now when p1 or p2 but not both (ie off the board)
		p = x & (turn == 1 ? p : p >> 1); // now just the selected player
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
	bool valid_move_fast(int x, int y)        const { return !get(x,y); }
	bool valid_move_fast(const Move & m)      const { return !get(m); }
	bool valid_move_fast(const MoveValid & m) const { return !get(m.xy); }
	//checks array bounds too
	bool valid_move(int x, int y)   const { return (outcome == -3 && onboard(x, y) && !get(x, y)); }
	bool valid_move(const Move & m) const { return (outcome == -3 && onboard(m) && !get(m)); }

	//iterator through neighbours of a position
	const MoveValid * nb_begin(int x, int y)   const { return nb_begin(xy(x, y)); }
	const MoveValid * nb_begin(const Move & m) const { return nb_begin(xy(m)); }
	const MoveValid * nb_begin(int i)          const { return &neighbourlist[i*18]; }

	const MoveValid * nb_end(int x, int y)   const { return nb_end(xy(x, y)); }
	const MoveValid * nb_end(const Move & m) const { return nb_end(xy(m)); }
	const MoveValid * nb_end(int i)          const { return nb_end(nb_begin(i)); }
	const MoveValid * nb_end(const MoveValid * m) const { return m + 6; }
	const MoveValid * nb_end_small_hood(const MoveValid * m) const { return m + 18; }
	const MoveValid * nb_end_big_hood(const MoveValid * m) const { return m + 12; }

	int edges(int x, int y) const {
		return (x == 0 ? 1 : 0) | (y == 0 ? 2 : 0) | (x + y == sizem1 ? 4 : 0);
	}

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


	int lineend(int y)   const { return (size - y); }

	string to_s(bool color) const {
		string white = "O",
		       black = "@",
		       empty = ".",
		       coord = "",
		       reset = "";
		if(color){
			string esc = "\033";
			reset = esc + "[0m";
			coord = esc + "[1;37m";
			empty = reset + ".";
			white = esc + "[1;33m" + "@"; //yellow
			black = esc + "[1;34m" + "@"; //blue
		}

		string s;
		for(int i = 0; i < size; i++)
			s += " " + coord + to_str(i+1);
		s += "\n";

		for(int y = 0; y < size; y++){
			s += string(y, ' ');
			s += coord + char('A' + y);
			int end = lineend(y);
			for(int x = 0; x < end; x++){
				s += (last == Move(x, y)   ? coord + "[" :
				      last == Move(x-1, y) ? coord + "]" : " ");
				int p = get(x, y);
				if(p == 0) s += empty;
				if(p == 1) s += white;
				if(p == 2) s += black;
				if(p >= 3) s += "?";
			}
			s += (last == Move(end-1, y) ? coord + "]" : " ");
			s += '\n';
		}

		s += reset;
		return s;
	}

	void print(bool color = true) const {
		printf("%s", to_s(color).c_str());
	}

	string boardstr() const {
		string white, black;
		for(int y = 0; y < size; y++){
			for(int x = 0; x < lineend(y); x++){
				int p = get(x, y);
				if(p == 1) white += Move(x, y).to_s();
				if(p == 2) black += Move(x, y).to_s();
			}
		}
		return white + ";" + black;
	}

	string won_str() const {
		switch(outcome){
			case -3: return "none";
			case -2: return "black_or_draw";
			case -1: return "white_or_draw";
			case 0:  return "draw";
			case 1:  return "white";
			case 2:  return "black";
		}
		return "unknown";
	}

	char won() const {
		return outcome;
	}

	int win() const{ // 0 for draw or unknown, 1 for win, -1 for loss
		if(outcome <= 0)
			return 0;
		return (outcome == toplay() ? 1 : -1);
	}

	char toplay() const {
		return toPlay;
	}

	MoveIterator moveit(bool unique = false) const {
		return MoveIterator(*this, (unique ? nummoves <= unique_depth : false));
	}

	void set(const Move & m, bool perm = true){
		last = m;
		Cell * cell = & cells[xy(m)];
		cell->piece = toPlay;
		cell->perm = perm;
		nummoves++;
		update_hash(m, toPlay); //depends on nummoves
		toPlay = 3 - toPlay;
	}

	void unset(const Move & m){ //break win checks, but is a poor mans undo if all you care about is the hash
		toPlay = 3 - toPlay;
		update_hash(m, toPlay);
		nummoves--;
		Cell * cell = & cells[xy(m)];
		cell->piece = 0;
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
			swap(i, j);

		cells[j].parent = i;
		cells[i].size   += cells[j].size;
		cells[i].edge   |= cells[j].edge;

		return false;
	}

	Cell test_cell(const Move & pos) const {
		char turn = toplay();
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

	string hashstr() const {
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

	void update_hash(const Move & pos, int turn){
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

	hash_t test_hash(const Move & pos, int turn) const {
		if(nummoves >= unique_depth) //simple test, no rotations/symmetry
			return hash.test(0, 3*xy(pos) + turn);

		int x = pos.x,
		    y = pos.y,
		    z = sizem1 - x - y;

		hash_t m = hash.test(0,  3*xy(x, y) + turn);
		m = min(m, hash.test(1,  3*xy(z, y) + turn));
		m = min(m, hash.test(2,  3*xy(z, x) + turn));
		m = min(m, hash.test(3,  3*xy(x, z) + turn));
		m = min(m, hash.test(4,  3*xy(y, z) + turn));
		m = min(m, hash.test(5,  3*xy(y, x) + turn));
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

	static Pattern pattern_invert(Pattern p){ //switch players
		return ((p & 0xAAAAAAAAAull) >> 1) | ((p & 0x555555555ull) << 1);
	}
	static Pattern pattern_rotate(Pattern p){
		return (((p & 0x003003003ull) << 10) | ((p & 0xFFCFFCFFC) >> 2));
	}
	static Pattern pattern_mirror(Pattern p){
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
	static Pattern pattern_symmetry(Pattern p){ //takes a pattern and returns the representative version
		Pattern m = p;                 //012345
		m = min(m, (p = pattern_rotate(p)));//501234
		m = min(m, (p = pattern_rotate(p)));//450123
		m = min(m, (p = pattern_rotate(p)));//345012
		m = min(m, (p = pattern_rotate(p)));//234501
		m = min(m, (p = pattern_rotate(p)));//123450
		m = min(m, (p = pattern_mirror(pattern_rotate(p))));//012345 -> 054321
		m = min(m, (p = pattern_rotate(p)));//105432
		m = min(m, (p = pattern_rotate(p)));//210543
		m = min(m, (p = pattern_rotate(p)));//321054
		m = min(m, (p = pattern_rotate(p)));//432105
		m = min(m, (p = pattern_rotate(p)));//543210
		return m;
	}

	bool move(const Move & pos){ return move(MoveValid(pos, xy(pos))); }
	bool move(const MoveValid & pos){
		assert(outcome < 0);

		if(!valid_move(pos))
			return false;

		char turn = toplay();
		set(pos);

		// update the nearby patterns
		Pattern p = turn;
		for(const MoveValid * i = nb_begin(pos.xy), *e = nb_end_big_hood(i); i < e; i++){
			if(i->onboard()){
				cells[i->xy].pattern |= p;
			}
			p <<= 2;
		}

		// join the groups for win detection
		for(const MoveValid * i = nb_begin(pos.xy), *e = nb_end(i); i < e; i++){
			if(i->onboard()){
				if(turn == get(i->xy)){
					join_groups(pos.xy, i->xy);
					i++; //skip the next one. If it is the same group,
						 //it is already connected and forms a corner, which we can ignore
				}
			}
		}

		// did I win?
		Cell * g = & cells[find_group(pos.xy)];
		if(g->numedges() == 3){
			outcome = turn;
		}

		return true;
	}

	bool test_local(const Move & pos, char turn) const {
		return (local(pos, turn) == 3);
	}

	//test if making this move would win, but don't actually make the move
	int test_win(const Move & pos, char turn = 0) const {
		if(turn == 0)
			turn = toplay();

		if(test_local(pos, turn)){
			int posxy = xy(pos);
			Cell testcell = cells[find_group(posxy)];
			int numgroups = 0;
			for(const MoveValid * i = nb_begin(posxy), *e = nb_end(i); i < e; i++){
				if(i->onboard() && turn == get(i->xy)){
					const Cell * g = & cells[find_group(i->xy)];
					testcell.edge   |= g->edge;
					testcell.size   += g->size;
					i++; //skip the next one
					numgroups++;
				}
			}

			if(testcell.numedges() == 3)
				return turn;
		}

		return -3;
	}
};
