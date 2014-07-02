
#pragma once

#include "../lib/exppair.h"

#include "board.h"
#include "move.h"

namespace Morat {
namespace Havannah {

struct MoveList {
	ExpPair    exp[2];       //aggregated outcomes overall
	ExpPair    rave[2][Board::max_vecsize]; //aggregated outcomes per move
	MovePlayer moves[Board::max_vecsize];   //moves made in order
	int        tree;         //number of moves in the tree
	int        rollout;      //number of moves in the rollout
	Board *    board;        //reference to rootboard for xy()

	MoveList() : tree(0), rollout(0), board(NULL) { }

	void addtree(const Move & move, Side player){
		moves[tree++] = MovePlayer(move, player);
	}
	void addrollout(const Move & move, Side player){
		moves[tree + rollout++] = MovePlayer(move, player);
	}
	void reset(Board * b){
		tree = 0;
		rollout = 0;
		board = b;
		exp[0].clear();
		exp[1].clear();
		for(int i = 0; i < b->vecsize(); i++){
			rave[0][i].clear();
			rave[1][i].clear();
		}
	}
	void finishrollout(Outcome won){
		exp[0].addloss();
		exp[1].addloss();
		if(won == Outcome::DRAW){
			exp[0].addtie();
			exp[1].addtie();
		}else{
			exp[won.to_i() - 1].addwin();

			for(MovePlayer * i = begin(), * e = end(); i != e; i++){
				ExpPair & r = rave[i->player.to_i() - 1][board->xy(*i)];
				r.addloss();
				if(+i->player == won)
					r.addwin();
			}
		}
		rollout = 0;
	}
	const MovePlayer * begin() const {
		return moves;
	}
	MovePlayer * begin() {
		return moves;
	}
	const MovePlayer * end() const {
		return moves + tree + rollout;
	}
	MovePlayer * end() {
		return moves + tree + rollout;
	}
	void subvlosses(int n){
		exp[0].addlosses(-n);
		exp[1].addlosses(-n);
	}
	const ExpPair & getrave(Side player, const Move & move) const {
		return rave[player.to_i() - 1][board->xy(move)];
	}
	const ExpPair & getexp(Side player) const {
		return exp[player.to_i() - 1];
	}
};

}; // namespace Havannah
}; // namespace Morat
