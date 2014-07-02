
#pragma once

#include <cstdint>
#include <string>

namespace Morat {

class Side {
	typedef int8_t Type;
	Type side;

public:

	static const Side UNDEF;
	static const Side NONE;
	static const Side P1;
	static const Side P2;
	static const Side BOTH;

	Side() : side(NONE.side) { }
	Side(const Type & s) : side(s) { }
	Side & operator=(const Type & s) {
		side = s;
		return *this;
	}

	std::string to_s() const;
	Type to_i() const { return side; }
};



// who has or will win if played perfectly
class Outcome {
	typedef int8_t Type;
	Type outcome;

public:

	// positive is that player has won or will win if played perfect
	// negative is that player might be able to win
	static const Outcome UNDEF;   // not yet computed
	static const Outcome UNKNOWN; // nothing known, could be either
	static const Outcome P2_DRAW; // player 2 or draw; player 1 can't win
	static const Outcome P1_DRAW; // player 1 or draw; player 2 can't win
	static const Outcome DRAW;    // draw, neither player can win
	static const Outcome P1;      // first player win
	static const Outcome P2;      // second player win
	static const Outcome DRAW2;   // draw by simultaneous win


	Outcome() : outcome(UNKNOWN.outcome) { }
	Outcome(const Type & o) : outcome(o) { }
	Outcome(const Side & s) : outcome(s.to_i()) { }
	Outcome & operator=(const Type & o) {
		outcome = o;
		return *this;
	}

	std::string to_s() const;
	std::string to_s_rel(Side to_play) const;
	Type to_i() const { return outcome; }
	bool cas(Outcome old, Outcome new_);
};



// switch to opponent
inline Side    operator~(Side    s) { return Side(3 - s.to_i()); }
// can't switch sides of an Outcome!
//: Outcome operator~(Outcome o) { return (Outcome)(3 - (int)o); }

// promote from Side to Outcome
inline Outcome operator+(Side s)    { return Outcome(s.to_i()); }

// this side can't win
inline Outcome operator-(Side s)    { return Outcome(s.to_i() - 3); }
inline Outcome operator-(Outcome o) { return Outcome(o.to_i() - 3); }


inline bool operator == (const Side    & a, const Side    & b) { return a.to_i() == b.to_i(); }
inline bool operator == (const Outcome & a, const Outcome & b) { return a.to_i() == b.to_i(); }
inline bool operator == (const Side    & a, const Outcome & b) { return a.to_i() == b.to_i(); }
inline bool operator == (const Outcome & a, const Side    & b) { return a.to_i() == b.to_i(); }

inline bool operator != (const Side    & a, const Side    & b) { return a.to_i() != b.to_i(); }
inline bool operator != (const Outcome & a, const Outcome & b) { return a.to_i() != b.to_i(); }
inline bool operator != (const Side    & a, const Outcome & b) { return a.to_i() != b.to_i(); }
inline bool operator != (const Outcome & a, const Side    & b) { return a.to_i() != b.to_i(); }

//inline bool operator >  (const Side    & a, const Side    & b) { return a.to_i() >  b.to_i(); }
//inline bool operator >= (const Side    & a, const Side    & b) { return a.to_i() >= b.to_i(); }
//inline bool operator <  (const Side    & a, const Side    & b) { return a.to_i() <  b.to_i(); }
//inline bool operator <= (const Side    & a, const Side    & b) { return a.to_i() <= b.to_i(); }

inline bool operator >  (const Outcome & a, const Outcome & b) { return a.to_i() >  b.to_i(); }
inline bool operator >= (const Outcome & a, const Outcome & b) { return a.to_i() >= b.to_i(); }
inline bool operator <  (const Outcome & a, const Outcome & b) { return a.to_i() <  b.to_i(); }
inline bool operator <= (const Outcome & a, const Outcome & b) { return a.to_i() <= b.to_i(); }


// for saying that P1 | P2 can win, meaning both can win
inline Side    operator|(const Side    & a, const Side    & b) { return Side(a.to_i() | b.to_i()); }
inline Outcome operator|(const Outcome & a, const Outcome & b) { return Outcome(a.to_i() | b.to_i()); }
inline Side    & operator|=(Side    & o, const Side    & s) { return o = o | s; }
inline Outcome & operator|=(Outcome & o, const Outcome & s) { return o = o | s; }

}; // namespace Morat
