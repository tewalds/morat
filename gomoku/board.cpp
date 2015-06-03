
#include "board.h"

namespace Morat {
namespace Gomoku {

std::string Board::Cell::to_s(int i) const {
	return "Cell " + to_str(i) +": "
		"piece: " + to_str(piece.to_i())+
		", pattern: " + to_str((int)pattern);
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
	s += ' ';
	for(int i = 0; i < size_x_; i++)
		s += " " + coord + to_str(i+1);
	s += "\n";

	for(int y = 0; y < size_y_; y++){
		s += coord + char('A' + y);
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


}; // namespace Gomoku
}; // namespace Morat
