
#include "board.h"

namespace Morat {
namespace Gomoku {

/*
 * the board is represented as a flattened 2d array of the form:
 *   1 2 3
 * A 0 1 2
 * B 3 4 5
 * C 6 7 8
 *
 * neighbors are laid out in this pattern:
 *  8  16 9 17 10
 * 23  0  1  2 18
 * 15  7  X  3 11
 * 22  6  5  4 19
 * 14 21 13 20 12
 */

const MoveScore neighbor_offsets[24] = {
	MoveScore(-1,-1, 3), MoveScore(0,-1, 3), MoveScore(1,-1, 3), MoveScore(1, 0, 3), MoveScore(1, 1, 3), MoveScore( 0, 1, 3), MoveScore(-1, 1, 3), MoveScore(-1, 0, 3), //direct neighbors, clockwise
	MoveScore(-2,-2, 2), MoveScore(0,-2, 2), MoveScore(2,-2, 2), MoveScore(2, 0, 2), MoveScore(2, 2, 2), MoveScore( 0, 2, 2), MoveScore(-2, 2, 2), MoveScore(-2, 0, 2), //corners
	MoveScore(-1,-2, 1), MoveScore(1,-2, 1), MoveScore(2,-1, 1), MoveScore(2, 1, 1), MoveScore(1, 2, 1), MoveScore(-1, 2, 1), MoveScore(-2, 1, 1), MoveScore(-2,-1, 1), //knight's move
};

const char* Board::win_names[] = {"5"};

std::string Board::Cell::to_s(int i) const {
	return "Cell " + to_str(i) +": "
		"piece: " + to_str(piece.to_i())+
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

	string s;
	s += "  ";
	for(int i = 0; i < size_; i++)
		s += " " + coord + char('A' + i);
	s += "\n";

	for(int y = 0; y < size_; y++){
		if (y + 1 < 10)
			s += ' ';
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

std::shared_ptr<MoveValid> Board::gen_neighbor_list() const {
	std::shared_ptr<MoveValid> list(new MoveValid[vec_size()*24]);
	MoveValid * a = list.get();
	for(int y = 0; y < size_; y++){
		for(int x = 0; x < size_; x++){
			Move pos(x,y);

			for(int i = 0; i < 24; i++){
				Move loc = pos + neighbor_offsets[i];
				*a = MoveValid(loc, (on_board(loc) ? xy(loc) : -1) );
				++a;
			}
		}
	}

	return list;
}

}; // namespace Gomoku
}; // namespace Morat
