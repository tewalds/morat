
#include "board.h"

namespace Morat {
namespace Hex {

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

	string s;
	for(int i = 0; i < size; i++)
		s += " " + coord + to_str(i+1);
	s += "\n";

	for(int y = 0; y < size; y++){
		s += string(y, ' ');
		s += coord + char('A' + y);
		int end = lineend(y);
		for(int x = linestart(y); x < end; x++){
			s += (last == Move(x, y)   ? coord + "[" :
			      last == Move(x-1, y) ? coord + "]" : " ");
			Side p = get(x, y);
			if(     p == Side::NONE) s += reset + func(Move(x, y));
			else if(p == Side::P1)   s += white;
			else if(p == Side::P2)   s += black;
			else                     s += "?";
		}
		s += (last == Move(end-1, y) ? coord + "]" : " ");
		s += white + reset;
		s += '\n';
	}
	s += string(size + 2, ' ');
	for(int i = 0; i < size; i++)
		s += black + " ";
	s += "\n";

	s += reset;
	return s;
}

int Board::edges(int x, int y) const {
	return (x == 0      ? 1 : 0) |
	       (x == sizem1 ? 2 : 0) |
	       (y == 0      ? 4 : 0) |
	       (y == sizem1 ? 8 : 0);
}


}; // namespace Hex
}; // namespace Morat
