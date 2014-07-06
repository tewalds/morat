
#pragma once

#include <cassert>
#include <cstdlib>
#include <stdint.h>

#include "../lib/log.h"
#include "../lib/string.h"


namespace Morat {
namespace Pentago {

enum MoveSpecial {
	M_SWAP    = -1, //-1 so that adding 1 makes it into a valid move
	M_RESIGN  = -2,
	M_NONE    = -3,
	M_UNKNOWN = -4,
};

struct Move {
	int8_t l; //location = MoveSpecial | y*6+x
	int8_t r; //rotation = 0-7

/* location =
 0  1  2  3  4  5
 6  7  8  9 10 11
12 13 14 15 16 17
18 19 20 21 22 23
24 25 26 27 28 29
30 31 32 33 34 35

rotation:
  1 2
0     3
7     4
  6 5

quadrants:
01
32

quadrant  = rotation >> 1
direction = rotation & 1
*/


	Move(MoveSpecial a = M_UNKNOWN) : l(a), r(14) { } //big r so it will always wrap to l=0 with swap
	Move(int L, unsigned int R) : l(L), r(R) { }
	Move(unsigned int X, unsigned int Y, unsigned int R) : l(Y*6 + X), r(R) { }

	Move(const std::string & str){
		if(     str == "swap"    ){ l = M_SWAP;    r = 14; }
		else if(str == "resign"  ){ l = M_RESIGN;  r = 14; }
		else if(str == "none"    ){ l = M_NONE;    r = 14; }
		else if(str == "unknown" ){ l = M_UNKNOWN; r = 14; }
		else if(str.length() <= 2){ l = M_NONE;    r = 14; }
		else{
			unsigned int y = tolower(str[0]) - 'a'; //[abcdef]
			unsigned int x = str[1] - '1';          //[123456]
			l = y*6+x;

			char c = tolower(str[2]);
			if(     c >= '1' && c <= '8') r = c - '1'; //[12345678]
			else if(c >= 's' && c <= 'z') r = c - 's'; //[stuvwxyz]
			else if(c >= 'a' && c <= 'h') r = c - 'a'; //[abcdefgh]
			else r = 0; //unknown, but do something
		}
	}

	int loc()                  const { return l; }
	unsigned int x()           const { return l % 6; }
	unsigned int y()           const { return l / 6; }
	unsigned int quadrant()    const { return r >> 1; }
	unsigned int direction()   const { return r & 1; }

	std::string to_s() const {
		if(l == M_UNKNOWN) return "unknown";
		if(l == M_NONE)    return "none";
		if(l == M_SWAP)    return "swap";
		if(l == M_RESIGN)  return "resign";

		return std::string() + char(y() + 'a') + to_str(x() + 1) + char(r + 's');
	}

	friend std::ostream& operator<< (std::ostream &out, const Move & m) { return out << m.to_s(); }

	bool operator< (const Move & b) const { return (l == b.l ? r <  b.r : l <  b.l); }
	bool operator<=(const Move & b) const { return (l == b.l ? r <= b.r : l <= b.l); }
	bool operator> (const Move & b) const { return (l == b.l ? r >  b.r : l >  b.l); }
	bool operator>=(const Move & b) const { return (l == b.l ? r >= b.r : l >= b.l); }
	bool operator==(const MoveSpecial & b) const { return (l == b); }
	bool operator==(const Move & b) const { return (l == b.l && r == b.r); }
	bool operator!=(const Move & b) const { return (l != b.l || r != b.r); }
	bool operator!=(const MoveSpecial & b) const { return (l != b); }
};

}; // namespace Pentago
}; // namespace Morat
