
#include "board.h"

namespace Morat {
namespace Y {

/*
 * the board is represented as a flattened 2d array of the form:
 *   1 2 3
 * A 0 1 2     0 1 2     0 1 2
 * B 3 4 5 <=> 3 4   <=>  3 4
 * C 6 7 8     6           6
 *
 * neighbors are laid out in this pattern:
 *     12   6  13               12  6 13
 *   11   0   1   7          11  0  1  7
 * 17   5   X   2  14 <=> 17  5  X  2 14
 *   10   4   3   8       10  4  3  8
 *     16   9  15         16  9 15
 */

const MoveScore neighbor_offsets[18] = {
	MoveScore( 0,-1, 3), MoveScore(1,-1, 3), MoveScore(1, 0, 3), MoveScore( 0, 1, 3), MoveScore(-1, 1, 3), MoveScore(-1, 0, 3), //direct neighbors, clockwise
	MoveScore( 1,-2, 2), MoveScore(2,-1, 2), MoveScore(1, 1, 2), MoveScore(-1, 2, 2), MoveScore(-2, 1, 2), MoveScore(-1,-1, 2), //sides of ring 2, virtual connections
	MoveScore( 0,-2, 1), MoveScore(2,-2, 1), MoveScore(2, 0, 1), MoveScore( 0, 2, 1), MoveScore(-2, 2, 1), MoveScore(-2, 0, 1), //corners of ring 2, easy to block
};

const char* Board::win_names[] = {"F"};

std::string Board::Cell::to_s(int i) const {
	return "Cell " + to_str(i) +": "
		"piece: " + to_str(piece.to_i())+
		", size: " + to_str((int)size) +
		", parent: " + to_str((int)parent) +
		", edge: " + to_str((int)edge) + "/" + to_str(numedges()) +
		", perm: " + to_str((int)perm) +
		", pattern: " + to_str((int)pattern);
}

std::string empty(Move m) { return "."; }

std::string Board::to_s(bool color) const {
	return to_s(color, empty);
}
std::string Board::to_s(bool color, std::function<std::string(Move)> func) const {
	using std::string;
	string white = "O",
	       black = "@",
	       coord = "",
	       reset = "";
	if(color){
		string esc = "\033";
		reset = esc + "[0m";
		coord = esc + "[1;37m";
		white = esc + "[1;33m" + "@"; //yellow
		black = esc + "[1;34m" + "@"; //blue
	}

	string s = " ";
	for(int i = 0; i < size_; i++)
		s += " " + coord + char('A' + i);
	s += "\n";

	for(int y = 0; y < size_; y++){
		s += string(y + ((y + 1) < 10), ' ');
		s += coord + to_str(y + 1);
		int end = line_end(y);
		for(int x = line_start(y); x < end; x++){
			s += (last_move_ == Move(x, y)   ? coord + "[" :
			      last_move_ == Move(x-1, y) ? coord + "]" : " ");
			Side p = get(x, y);
			if(     p == Side::NONE) s += reset + func(Move(x, y));
			else if(p == Side::P1)   s += white;
			else if(p == Side::P2)   s += black;
			else                     s += "?";
		}
		s += (last_move_ == Move(end-1, y) ? coord + "]" : " ");
		s += '\n';
	}

	s += reset;
	return s;
}

int Board::edges(int x, int y) const {
	return (x == 0 ? 1 : 0) |
	       (y == 0 ? 2 : 0) |
	       (x + y == sizem1_ ? 4 : 0);
}

std::shared_ptr<MoveValid> Board::gen_neighbor_list() const {
	std::shared_ptr<MoveValid> list(new MoveValid[vec_size()*18]);
	MoveValid * a = list.get();
	for(int y = 0; y < size_; y++){
		for(int x = 0; x < size_; x++){
			Move pos(x,y);

			for(int i = 0; i < 18; i++){
				Move loc = pos + neighbor_offsets[i];
				*a = MoveValid(loc, (on_board(loc) ? xy(loc) : -1) );
				++a;
			}
		}
	}

	return list;
}

}; // namespace Y
}; // namespace Morat
