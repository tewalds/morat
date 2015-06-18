
#pragma once

#include "../lib/lbdist.h"

#include "board.h"


namespace Morat {
namespace Y {

class LBDists : public LBDistsBase<LBDists, Board> {

public:
	LBDists() : LBDistsBase() {}
	LBDists(const Board * b) : LBDistsBase(b) { }

	void init_player(bool crossvcs, Side player){
		int m = board->lines();
		int m1 = m-1;

		for(int x = 0; x < m; x++) { init(x,    0, 0,  player, 3); } flood(0,  player, crossvcs); //edge 0
		for(int y = 0; y < m; y++) { init(0,    y, 1,  player, 1); } flood(1,  player, crossvcs); //edge 1
		for(int y = 0; y < m; y++) { init(m1-y, y, 2,  player, 5); } flood(2,  player, crossvcs); //edge 2
	}

	int _get(int pos, Side player){
		int sum = 0;
		for(int i = 0; i < 3; i++)
			sum += dist(i, player, pos);
		return sum;
	}
};

}; // namespace Y
}; // namespace Morat
