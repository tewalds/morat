
#include "board.h"

namespace Morat {
namespace Havannah {

std::string Board::to_s(bool color) const {
	using std::string;
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
	s += string(size + 3, ' ');
	for(int i = 0; i < size; i++)
		s += " " + coord + to_str(i+1);
	s += "\n";

	for(int y = 0; y < size_d; y++){
		s += string(abs(sizem1 - y) + 2, ' ');
		s += coord + char('A' + y);
		int end = lineend(y);
		for(int x = linestart(y); x < end; x++){
			s += (last == Move(x, y)   ? coord + "[" :
			      last == Move(x-1, y) ? coord + "]" : " ");
			Side p = get(x, y);
			if(     p == Side::NONE) s += empty;
			else if(p == Side::P1)   s += white;
			else if(p == Side::P2)   s += black;
			else                     s += "?";
		}
		s += (last == Move(end-1, y) ? coord + "]" : " ");
		if(y < sizem1)
			s += coord + to_str(size + y + 1);
		s += '\n';
	}

	s += reset;
	return s;
}

}; // namespace Havannah
}; // namespace Morat
