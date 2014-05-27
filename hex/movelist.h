
#pragma once

#include "../lib/exppair.h"

#include "board.h"
#include "move.h"

struct MoveList {
	ExpPair    exp[2];       //aggregated outcomes overall
	ExpPair    rave[2][256]; //aggregated outcomes per move
	MovePlayer moves[4096];   //moves made in order
	int        tree;         //number of moves in the tree
	int        rollout;      //number of moves in the rollout
	Board *    board;        //reference to rootboard for xy()

	MoveList() : tree(0), rollout(0), board(NULL) { }

	void addtree(const Move & move, char player){
		moves[tree++] = MovePlayer(move, player);
	}
	void addrollout(const Move & move, char player){
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
	void finishrollout(int won){
		exp[0].addloss();
		exp[1].addloss();
		if(won == 0){
			exp[0].addtie();
			exp[1].addtie();
		}else{
			exp[won-1].addwin();

			for(MovePlayer * i = begin(), * e = end(); i != e; i++){
				ExpPair & r = rave[i->player-1][board->xy(*i)];
				r.addloss();
				if(i->player == won)
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
	const ExpPair & getrave(int player, const Move & move) const {
		return rave[player-1][board->xy(move)];
	}
	const ExpPair & getexp(int player) const {
		return exp[player-1];
	}
};
