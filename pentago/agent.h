
#pragma once

//Interface for the various agents: players and solvers

#include "../lib/outcome.h"
#include "../lib/sgf.h"
#include "../lib/types.h"

#include "board.h"
#include "moveiterator.h"


namespace Morat {
namespace Pentago {

class Agent {
protected:
	typedef std::vector<Move> vecmove;
public:
	Agent() { }
	virtual ~Agent() { }

	virtual void search(double time, uint64_t maxruns, int verbose) = 0;
	virtual Move return_move(int verbose) const = 0;
	virtual void set_board(const Board & board, bool clear = true) = 0;
	virtual void move(const Move & m) = 0;
	virtual void set_memlimit(uint64_t lim) = 0; // in bytes
	virtual void clear_mem() = 0;

	virtual vecmove get_pv() const = 0;
	        std::string move_stats() const { return move_stats(vecmove()); }
	virtual std::string move_stats(const vecmove moves) const = 0;
	virtual double gamelen() const = 0;

	virtual void timedout(){ timeout = true; }

	virtual void gen_sgf(SGFPrinter<Move> & sgf, int limit) const = 0;
	virtual void load_sgf(SGFParser<Move> & sgf) = 0;

protected:
	volatile bool timeout;
	Board rootboard;

	static Outcome solve1ply(const Board & board, unsigned int & nodes) {
		Outcome outcome = Outcome::UNDEF;
		Side turn = board.toplay();
		for(MoveIterator move(board); !move.done(); ++move){
			++nodes;
			Outcome won = move.board().won();

			if(won == +turn)
				return won;
			if(won == Outcome::DRAW)
				outcome = Outcome::DRAW;
			else if(outcome == +~turn || outcome == Outcome::UNDEF)
				outcome = won;
		}
		return outcome;
	}
};

}; // namespace Pentago
}; // namespace Morat
