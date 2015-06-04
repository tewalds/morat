
#include "board.h"

namespace Morat {
namespace Havannah {

std::string Board::Cell::to_s(int i) const {
	return "Cell " + to_str(i) +": "
		"piece: " + to_str(piece.to_i())+
		", size: " + to_str((int)size) +
		", parent: " + to_str((int)parent) +
		", corner: " + to_str((int)corner) + "/" + to_str(numcorners()) +
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
	s += string(size + 3, ' ');
	for(int i = 0; i < size; i++)
		s += " " + coord + to_str(i+1);
	s += "\n";

	for(int y = 0; y < size_d; y++){
		s += string(abs(sizem1 - y) + 2, ' ');
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
		if(y < sizem1)
			s += coord + to_str(size + y + 1);
		s += '\n';
	}

	s += reset;
	return s;
}


int Board::iscorner(int x, int y) const {
	if(!on_board(x,y))
		return -1;

	int m = sizem1, e = size_d-1;

	if(x == 0 && y == 0) return 0;
	if(x == m && y == 0) return 1;
	if(x == e && y == m) return 2;
	if(x == e && y == e) return 3;
	if(x == m && y == e) return 4;
	if(x == 0 && y == m) return 5;

	return -1;
}

int Board::isedge(int x, int y) const {
	if(!on_board(x,y))
		return -1;

	int m = sizem1, e = size_d-1;

	if(y   == 0 && x != 0 && x != m) return 0;
	if(x-y == m && x != m && x != e) return 1;
	if(x   == e && y != m && y != e) return 2;
	if(y   == e && x != e && x != m) return 3;
	if(y-x == m && x != m && x != 0) return 4;
	if(x   == 0 && y != m && y != 0) return 5;

	return -1;
}

// do a depth first search for a ring
bool Board::checkring_df(const MoveValid & pos, const Side turn) const {
	const Cell * start = cell(pos);
	start->mark = 1;
	bool success = false;
	for(int i = 0; i < 4; i++){ //4 instead of 6 since any ring must have its first endpoint in the first 4
		MoveValid loc = nb_begin(pos)[i];

		if(!loc.on_board())
			continue;

		const Cell * g = cell(loc);

		if(turn != g->piece)
			continue;

		g->mark = 1;
		success = followring(loc, i, turn, (perm_rings - g->perm));
		g->mark = 0;

		if(success)
			break;
	}
	start->mark = 0;
	return success;
}
// only take the 3 directions that are valid in a ring
// the backwards directions are either invalid or not part of the shortest loop
bool Board::followring(const MoveValid & cur, const int & dir, const Side & turn, const int & permsneeded) const {
	for(int i = 5; i <= 7; i++){
		int nd = (dir + i) % 6;
		MoveValid next = nb_begin(cur)[nd];

		if(!next.on_board())
			continue;

		const Cell * g = cell(next);

		if(g->mark)
			return (permsneeded <= 0);

		if(turn != g->piece)
			continue;

		g->mark = 1;
		bool success = followring(next, nd, turn, (permsneeded - g->perm));
		g->mark = 0;

		if(success)
			return true;
	}
	return false;
}

// do an O(1) ring check
// must be done before placing the stone and joining it with the neighboring groups
bool Board::checkring_o1(const MoveValid & pos, const Side turn) const {
	static const unsigned char ringdata[64][10] = {
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //000000
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //000001
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //000010
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //000011
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //000100
		{1, 3, 5, 0, 0, 0, 0, 0, 0, 0}, //000101
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //000110
		{3,16,10, 9, 0, 0, 0, 0, 0, 0}, //000111
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //001000
		{1, 2, 5, 0, 0, 0, 0, 0, 0, 0}, //001001
		{1, 2, 4, 0, 0, 0, 0, 0, 0, 0}, //001010
		{1, 2, 4, 0, 0, 0, 0, 0, 0, 0}, //001011
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //001100
		{1, 2, 5, 0, 0, 0, 0, 0, 0, 0}, //001101
		{3,15, 9, 8, 0, 0, 0, 0, 0, 0}, //001110
		{4,16,10, 9,15, 8, 9, 0, 0, 0}, //001111
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //010000
		{1, 1, 5, 0, 0, 0, 0, 0, 0, 0}, //010001
		{1, 1, 4, 0, 0, 0, 0, 0, 0, 0}, //010010
		{1, 1, 4, 0, 0, 0, 0, 0, 0, 0}, //010011
		{1, 1, 3, 0, 0, 0, 0, 0, 0, 0}, //010100
		{2, 1, 3, 5, 0, 0, 0, 0, 0, 0}, //010101
		{1, 1, 3, 0, 0, 0, 0, 0, 0, 0}, //010110
		{7,16,10, 9, 1, 3, 0, 0, 0, 0}, //010111
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //011000
		{1, 1, 5, 0, 0, 0, 0, 0, 0, 0}, //011001
		{1, 1, 4, 0, 0, 0, 0, 0, 0, 0}, //011010
		{1, 1, 4, 0, 0, 0, 0, 0, 0, 0}, //011011
		{3,14, 8, 7, 0, 0, 0, 0, 0, 0}, //011100
		{7,14, 8, 7, 1, 5, 0, 0, 0, 0}, //011101
		{4,15, 9, 8,14, 7, 8, 0, 0, 0}, //011110
		{5,16,10, 9,15, 8, 9,14, 8, 7}, //011111
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //100000
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //100001
		{1, 0, 4, 0, 0, 0, 0, 0, 0, 0}, //100010
		{3,17,11,10, 0, 0, 0, 0, 0, 0}, //100011
		{1, 0, 3, 0, 0, 0, 0, 0, 0, 0}, //100100
		{1, 0, 3, 0, 0, 0, 0, 0, 0, 0}, //100101
		{1, 0, 3, 0, 0, 0, 0, 0, 0, 0}, //100110
		{4,17,11,10,16, 9,10, 0, 0, 0}, //100111
		{1, 0, 2, 0, 0, 0, 0, 0, 0, 0}, //101000
		{1, 0, 2, 0, 0, 0, 0, 0, 0, 0}, //101001
		{2, 0, 2, 4, 0, 0, 0, 0, 0, 0}, //101010
		{7,17,11,10, 0, 2, 0, 0, 0, 0}, //101011
		{1, 0, 2, 0, 0, 0, 0, 0, 0, 0}, //101100
		{1, 0, 2, 0, 0, 0, 0, 0, 0, 0}, //101101
		{7,15, 9, 8, 0, 2, 0, 0, 0, 0}, //101110
		{5,17,11,10,16, 9,10,15, 9, 8}, //101111
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //110000
		{3,12, 6,11, 0, 0, 0, 0, 0, 0}, //110001
		{1, 0, 4, 0, 0, 0, 0, 0, 0, 0}, //110010
		{4,12, 6,11,17,10,11, 0, 0, 0}, //110011
		{1, 0, 3, 0, 0, 0, 0, 0, 0, 0}, //110100
		{7,12, 6,11, 0, 3, 0, 0, 0, 0}, //110101
		{1, 0, 3, 0, 0, 0, 0, 0, 0, 0}, //110110
		{5,12, 6,11,17,10,11,16,10, 9}, //110111
		{3,13, 7, 6, 0, 0, 0, 0, 0, 0}, //111000
		{4,13, 7, 6,12,11, 6, 0, 0, 0}, //111001
		{7,13, 7, 6, 0, 4, 0, 0, 0, 0}, //111010
		{5,13, 7, 6,12,11, 6,17,11,10}, //111011
		{4,14, 8, 7,13, 6, 7, 0, 0, 0}, //111100
		{5,14, 8, 7,13, 6, 7,12, 6,11}, //111101
		{5,15, 9, 8,14, 7, 8,13, 7, 6}, //111110
		{6, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //111111
	};

	int bitpattern = 0;
	const MoveValid * s = nb_begin(pos);
	for(const MoveValid * i = s, *e = nb_end(i); i < e; i++){
		bitpattern <<= 1;
		if(i->on_board() && turn == get(i->xy))
			bitpattern |= 1;
	}

	const unsigned char * d = ringdata[bitpattern];

	switch(d[0]){
		case 0: //no ring (000000, 000001, 000011)
			return false;

		case 1: //simple case (000101, 001101, 001011, 011011)
			return (find_group(s[d[1]]) == find_group(s[d[2]]));

		case 2:{ //3 non-neighbors (010101)
			int a = find_group(s[d[1]]), b = find_group(s[d[2]]), c = find_group(s[d[3]]);
			return (a == b || a == c || b == c);
		}

		case 7: //case 1 and 3 (010111)
			if(find_group(s[d[4]]) == find_group(s[d[5]]))
				return true;
			//fall through

		case 3: // 3 neighbors (000111)
			return checkring_back(s[d[1]], s[d[2]], s[d[3]], turn);

		case 4: // 4 neighbors (001111)
			return checkring_back(s[d[1]], s[d[2]], s[d[3]], turn) ||
			       checkring_back(s[d[4]], s[d[5]], s[d[6]], turn);

		case 5: // 5 neighbors (011111)
			return checkring_back(s[d[1]], s[d[2]], s[d[3]], turn) ||
			       checkring_back(s[d[4]], s[d[5]], s[d[6]], turn) ||
			       checkring_back(s[d[7]], s[d[8]], s[d[9]], turn);

		case 6: // 6 neighbors (111111)
			return true; //a ring around this position? how'd that happen

		default:
			return false;
	}
}
//checks for 3 more stones, a should be the corner
bool Board::checkring_back(const MoveValid & a, const MoveValid & b, const MoveValid & c, Side turn) const {
	return (a.on_board() && get(a) == turn && get(b) == turn && get(c) == turn);
}


}; // namespace Havannah
}; // namespace Morat
