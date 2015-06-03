
#pragma once

#include "hashset.h"
#include "move.h"
#include "types.h"

namespace Morat {

class BaseBoard;

class MoveIterator {
protected:
	const BaseBoard& board;
	MoveValid move;
	i8 lineend;

public:
	MoveIterator(const BaseBoard& b) : board(b), move(Move(M_SWAP), -1) {
		++(*this); //find the first valid move
	}
	MoveIterator(const BaseBoard& b, MoveValid m) : board(b), move(m) { }
	virtual ~MoveIterator() {}
	const MoveValid & operator * ()  const { return move; }
	const MoveValid * operator -> () const { return & move; }
	bool operator == (const MoveIterator & rhs) const { return (move == rhs.move); }
	bool operator != (const MoveIterator & rhs) const { return (move != rhs.move); }
	MoveIterator & operator ++ (); //prefix form
};


class MoveIteratorUnique : public MoveIterator {
	HashSet hashes;
public:
	MoveIteratorUnique(const BaseBoard & b);
	MoveIteratorUnique & operator ++ ();
};

/*
template<class Rand>
class MoveIteratorRandom : public MoveIterator {
	Rand & rand;
	std::vector<MoveValid> moves;

public:
	MoveIteratorRandom(const BaseBoard & b, Rand & r);
	MoveIteratorRandom & operator ++ ();
};
*/
}; // namespace Morat
