
#include <cassert>

#include "outcome.h"
#include "thread.h"

namespace Morat {

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


std::ostream& operator<< (std::ostream &out, const Side & s) {
	return out << s.to_s();
}

std::ostream& operator<< (std::ostream &out, const Outcome & o) {
	return out << o.to_s();
}

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

}; // namespace Morat
