
#include <cassert>

#include "outcome.h"
#include "thread.h"

const Side Side::UNDEF = -4;
const Side Side::NONE  = 0;
const Side Side::P1    = 1;
const Side Side::P2    = 2;
const Side Side::BOTH  = 3;

const Outcome Outcome::UNDEF   = -4; // not yet computed
const Outcome Outcome::UNKNOWN = -3; // nothing known, could be either
const Outcome Outcome::P2_DRAW = -2; // player 2 or draw; player 1 can't win
const Outcome Outcome::P1_DRAW = -1; // player 1 or draw; player 2 can't win
const Outcome Outcome::DRAW    = 0;  // draw, neither player can win
const Outcome Outcome::P1      = 1;  // first player win
const Outcome Outcome::P2      = 2;  // second player win
const Outcome Outcome::DRAW2   = 3;  // draw by simultaneous win



std::string Side::to_s() const {
	if(*this == UNDEF) return "never";
	if(*this == NONE)  return "none";
	if(*this == P1)    return "white";
	if(*this == P2)    return "black";
	if(*this == BOTH)  return "both";
	                   return "unknown";
}

std::string Outcome::to_s() const {
	if(*this == UNDEF)   return "undefined";
	if(*this == UNKNOWN) return "unknown";
	if(*this == P2_DRAW) return "black_or_draw";
	if(*this == P1_DRAW) return "white_or_draw";
	if(*this == DRAW ||
	   *this == DRAW2)   return "draw"; // simultaneous win
	if(*this == P1)      return "white";
	if(*this == P2)      return "black";
	                     return "unknown";
}

std::string Outcome::to_s_rel(Side to_play) const {
	if(*this == Outcome::DRAW)  return "draw";
	if(*this == Outcome::DRAW2) return "draw by simultaneous win";
	if(*this == +to_play)        return "win";
	if(*this == +~to_play)       return "loss";
	if(*this == -~to_play)      return "win or draw";
	if(*this == -to_play)       return "loss or draw";
	return "unkown";
}

bool Outcome::cas(Outcome old, Outcome new_) {
	return CAS(outcome, old.outcome, new_.outcome);
}



void outcome_test() {
	assert(~Side::P1    == Side::P2);
	assert(~Side::P2    == Side::P1);

	assert(+Side::NONE == Outcome::DRAW); // neither side wins => draw
	assert(+Side::P1   == Outcome::P1);
	assert(+Side::P2   == Outcome::P2);
	assert(+Side::BOTH == Outcome::DRAW2); // both sides win => draw

	assert(-Side::NONE  == Outcome::UNKNOWN); // neither side is known to not win => unknown
	assert(-Side::P1    == Outcome::P2_DRAW); // p1 can't win => p2 can win or draw
	assert(-Side::P2    == Outcome::P1_DRAW); // p2 can't win => p1 can win or draw
	assert(-Side::BOTH  == Outcome::DRAW);    // both sides can't win => no one can win => draw
	assert(-Outcome::P1 == Outcome::P2_DRAW);
	assert(-Outcome::P2 == Outcome::P1_DRAW);

	assert(-~Side::P1   == Outcome::P1_DRAW);
	assert(-~Side::P2   == Outcome::P2_DRAW);

	// invalid! wrong order! ~ and - are not commutative
	//: assert(~-Side::P1   == Outcome::P1_DRAW);

	assert(Side::P1 == Outcome::P1);
	assert(Side::P2 == Outcome::P2);
	assert(Outcome::P1 == Side::P1);
	assert(Outcome::P2 == Side::P2);
	assert(Side::P1 != Outcome::P2);
	assert(Side::P2 != Outcome::P1);
	assert(Outcome::P1 != Side::P2);
	assert(Outcome::P2 != Side::P1);

	assert((Side::P1    | Side::P2)    == Side::BOTH);
	assert((Outcome::P1 | Outcome::P2) == Outcome::DRAW2);
}
