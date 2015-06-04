
#pragma once

#include "../lib/lbdist.h"

#include "board.h"


namespace Morat {
namespace Havannah {

class LBDists : public LBDistsBase<LBDists, Board> {

public:
	LBDists() : LBDistsBase() {}
	LBDists(const Board * b) : LBDistsBase(b) { }

	void init_player(bool crossvcs, Side player) {
		int m = board->get_size()-1, e = board->get_size_d()-1;

		init(0, 0, 0, player, 3); flood(0, player, crossvcs); //corner 0
		init(m, 0, 1, player, 4); flood(1, player, crossvcs); //corner 1
		init(e, m, 2, player, 5); flood(2, player, crossvcs); //corner 2
		init(e, e, 3, player, 0); flood(3, player, crossvcs); //corner 3
		init(m, e, 4, player, 1); flood(4, player, crossvcs); //corner 4
		init(0, m, 5, player, 2); flood(5, player, crossvcs); //corner 5

		for(int x = 1; x < m; x++)   { init(x,   0, 6,  player, 3+(x==1));   } flood(6,  player, crossvcs); //edge 0
		for(int y = 1; y < m; y++)   { init(m+y, y, 7,  player, 4+(y==1));   } flood(7,  player, crossvcs); //edge 1
		for(int y = m+1; y < e; y++) { init(e,   y, 8,  player, 5+(y==m+1)); } flood(8,  player, crossvcs); //edge 2
		for(int x = m+1; x < e; x++) { init(x,   e, 9,  player, 0+(x==e-1)); } flood(9,  player, crossvcs); //edge 3
		for(int x = 1; x < m; x++)   { init(x, m+x, 10, player, 1+(x==m-1)); } flood(10, player, crossvcs); //edge 4
		for(int y = 1; y < m; y++)   { init(0,   y, 11, player, 2+(y==m-1)); } flood(11, player, crossvcs); //edge 5
	}

	Outcome isdraw() {
		Outcome outcome = Outcome::DRAW;  // assume neither side can win
		for(int y = 0; y < board->get_size_d(); y++) {
			for(int x = board->line_start(y); x < board->line_end(y); x++) {
				MoveValid pos(x, y, board->xy(x, y));

				if(board->encirclable(pos, Side::P1) || get(pos.xy, Side::P1) < maxdist-5)
					outcome |= Outcome::P1; // P1 can win
				if(board->encirclable(pos, Side::P2) || get(pos.xy, Side::P2) < maxdist-5)
					outcome |= Outcome::P2; // P2 can win

				if(outcome == Outcome::DRAW2) // both can win
					return Outcome::UNKNOWN;  // so nothing is known
			}
		}
		return -outcome; // this isn't certainty, so negate
	}

	int _get(int pos, Side player) {
		int list[6];
		for(int i = 0; i < 6; i++)
			list[i] = dist(i, player, pos);
		partialsort(list, 2);
		int corners = list[0] + list[1] - 1;

		for(int i = 6; i < 12; i++)
			list[i-6] = dist(i, player, pos);
		partialsort(list, 3);
		int edges = list[0] + list[1] + list[2] - 2;

		return std::min(corners, edges);
	}
};

}; // namespace Havannah
}; // namespace Morat
