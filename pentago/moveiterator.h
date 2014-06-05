
#pragma once

#include "../lib/hashset.h"

#include "board.h"
#include "move.h"

class MoveIterator { //only returns valid moves...
	const Board & base_board; //base board
	Board after; // board after making the move
	Move move;
	bool unique;
	HashSet hashes;
public:
	MoveIterator(const Board & b, int Unique = -1) : base_board(b), move(M_SWAP) {
		unique = (Unique == -1 ? base_board.num_moves() <= Board::unique_depth : Unique);

		if(base_board.won() >= 0){
			move = Move(36, 8); //already done
		} else {
			if(unique)
				hashes.init(base_board.moves_avail());
			++(*this); //find the first valid move
		}
	}

	const Board & board() const { return after; }
	const Move & operator * ()  const { return move; }
	const Move * operator -> () const { return & move; }
	bool done() const { return (move.l >= 36); }
	MoveIterator & operator ++ (){ //prefix form
		while(true){
			move.r++;
			if(move.r >= 8){
				move.r = 0;
				do{
					move.l++;
					if(move.l >= 36) //done
						return *this;
				}while(!base_board.valid_move_fast(move));
			}
			after = base_board;
			bool move_success = after.move(move);
			assert(move_success);
			if(unique){
				uint64_t h = after.full_hash();
				if(!hashes.add(h))
					continue;
			}
			break;
		}
		return *this;
	}

	static void test();
};


template<class Rand>
class RandomMoveIterator { //only returns valid moves...
	const Board & base_board; //base board
	Board after; // board after making the move
	Rand & rand;
	Move * moves;
	int cur;

	void choose_rand(){
		// incremental fisher-yates shuffle. Choose a random element that hasn't yet been chosen.
		int j = rand() % (cur+1);
		Move tmp = moves[j];
		moves[j] = moves[cur];
		moves[cur] = tmp;
//		logerr(moves[cur].to_s() + " ");
	}

public:
	RandomMoveIterator(const Board & b, Rand & r, int Unique = -1) : base_board(b), rand(r), moves(NULL), cur(0) {
		moves = new Move[b.moves_avail()];

		for(MoveIterator it(b, Unique); !it.done(); ++it)
			moves[cur++] = *it;

//		logerr("\n");
		++(*this); //find the first valid move
	}
	~RandomMoveIterator(){
		if(moves)
			delete[] moves;
		moves = NULL;
	}

	const Board & board() const { return after; }
	const Move & operator * ()  const { return moves[cur]; }
	const Move * operator -> () const { return & moves[cur]; }
	bool done() const { return cur < 0; }
	RandomMoveIterator & operator ++ (){ //prefix form
		cur--;
		if(cur >= 0){
			choose_rand();
			after = base_board;
			bool move_success = after.move(moves[cur]);
			assert(move_success);
		}
		return *this;
	}
};

void RandomMoveIteratorTest();
