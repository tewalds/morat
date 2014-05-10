
#include <set>

#include "../lib/string.h"
#include "../lib/xorshift.h"

#include "board.h"
#include "moveiterator.h"


void MoveIterator::test() {
//	printf("MoveIterator tests\n");

	Board board;
	int i;

	i = 0;
	for(MoveIterator move(board, 0); !move.done(); ++move)
		i++;
	assert(i == 288);

	i = 0;
	for(MoveIterator move(board, 36); !move.done(); ++move)
		i++;
	assert(i == 6);

	i = 0;
	for(MoveIterator move(board); !move.done(); ++move)
		i++;
	assert(i == 6);
}

void RandomMoveIteratorTest() {
	XORShift_uint32 rand32;
	XORShift_uint64 rand64;

	Board board;
	int i;

	i = 0;
	for (RandomMoveIterator<XORShift_uint32> move(board, rand32, 0); !move.done(); ++move)
		i++;
	assert(i == 288);

	i = 0;
	for (RandomMoveIterator<XORShift_uint64> move(board, rand64, 0); !move.done(); ++move)
		i++;
	assert(i == 288);

	i = 0;
	for (RandomMoveIterator<XORShift_uint32> move(board, rand32, 36); !move.done(); ++move)
		i++;
	assert(i == 6);

	i = 0;
	for (RandomMoveIterator<XORShift_uint32> move(board, rand32); !move.done(); ++move)
		i++;
	assert(i == 6);

	set<string> moves;
	for(MoveIterator move(board, 0); !move.done(); ++move)
		moves.insert(move->to_s());

	assert(moves.size() == 288);

	for (RandomMoveIterator<XORShift_uint32> move(board, rand32, 0); !move.done(); ++move)
		moves.insert(move->to_s());

	assert(moves.size() == 288);

	for (RandomMoveIterator<XORShift_uint32> move(board, rand32, 0); !move.done(); ++move)
		moves.erase(move->to_s());

	assert(moves.size() == 0);


	set<string> boards;
	for(MoveIterator move(board, 0); !move.done(); ++move)
		boards.insert(move.board().to_s());

	unsigned int size = boards.size();

	for (RandomMoveIterator<XORShift_uint32> move(board, rand32, 0); !move.done(); ++move)
		boards.insert(move.board().to_s());

	assert(boards.size() == size);

	for (RandomMoveIterator<XORShift_uint32> move(board, rand32, 0); !move.done(); ++move)
		boards.erase(move.board().to_s());

	assert(boards.size() == 0);
}
