 
#pragma once

#include <algorithm>
#include <string>

namespace Morat {

struct TimeControl {
	enum Method { PERCENT, EVEN, STATS };
	Method method;   // method to use to distribute the remaining time
	double param;    // param for the method, such as the percentage used or multiple of even
	double game;     // how much time per side per game
	double move;     // how much is added per move
	bool   flexible; // whether time_per_move can be saved for future moves
	int    max_sims; // maximum number of simulations
	double remain;   // how much time is remaining this game

	TimeControl(){
		method   = STATS;
		param    = 2;
		game     = 0;
		move     = 5;
		flexible = true;
		max_sims = 0;
		remain   = game;
	}

	std::string method_name(){
		switch(method){
			case PERCENT: return "percent";
			case EVEN:    return "even";
			case STATS:   return "stats";
			default:      return "WTF? unknown time control method";
		}
	}

	void new_game() {
		remain = game;
	}

	double get_time(int max_moves_remaining, int moves_remain_estimate) {
		double ret = 0;

		switch(method){
			case PERCENT:
				ret += param * remain / 100.0;
				break;
			case STATS:
				if(moves_remain_estimate > 0){
					ret += 2.0 * param * remain / moves_remain_estimate;
					break;
				}//fall back to even
			case EVEN:
				ret += 2.0 * param * remain / max_moves_remaining;
				break;
		}

		if(ret > remain)
			ret = remain;

		ret += move;

		return ret;
	}

	void use(double used){
		if(flexible)
			remain += move - used;
		else
			remain += std::min(0.0, move - used);

		if(remain < 0)
			remain = 0;
	}
};

}; // namespace Morat
