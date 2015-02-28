
#pragma once

#include "../lib/lbdist.h"

#include "board.h"


namespace Morat {
namespace Hex {

class LBDists : public LBDistsBase<LBDists, Board> {

public:
	LBDists() : LBDistsBase() {}
	LBDists(const Board * b) : LBDistsBase(b) { }

	void init_player(bool crossvcs, Side player){
		int m = board->get_size();
		int m1 = m-1;

		if (player == Side::P1) {
			for(int y = 0; y < m; y++) { init(0,  y, 0,  player, 2); } flood(0,  player, crossvcs);  // p1 edge 0
			for(int y = 0; y < m; y++) { init(m1, y, 1,  player, 5); } flood(1,  player, crossvcs);  // p1 edge 1
		} else {
			for(int x = 0; x < m; x++) { init(x,  0, 0,  player, 3); } flood(0,  player, crossvcs);  // p2 edge 0
			for(int x = 0; x < m; x++) { init(x, m1, 1,  player, 0); } flood(1,  player, crossvcs);  // p2 edge 1
		}
	}

	int _get(int pos, Side player){
		return dist(0, player, pos) + dist(1, player, pos);
	}
};

}; // namespace Hex
}; // namespace Morat
