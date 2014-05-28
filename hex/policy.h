
#pragma once

#include "board.h"
#include "move.h"
#include "movelist.h"

class Policy {
public:
	Policy() { }

	// called before all the rollouts start
	void prepare(const Board & board) { }

	// called at the beginning of each rollout.
	void rollout_start(Board & board) { }

	// Give me a move to make, or M_UNKNOWN
	Move choose_move(const Board & board, const Move & prev) {
		return M_UNKNOWN;
	}

	// A move was just made, here's the updated board
	void move_end(const Board & board, const Move & prev) { }

	// Game over, here's who won
	void rollout_end(const MoveList & movelist, int won) { }
};
