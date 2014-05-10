
#pragma once

#include "thread.h"
#include "types.h"

class ExpPair {
	uword s, n;
	ExpPair(uword S, uword N) : s(S), n(N) { }
public:
	ExpPair() : s(0), n(0) { }
	float avg() const { return 0.5f*s/n; }
	uword num() const { return n; }
	uword sum() const { return s/2; }

	void clear() { s = 0; n = 0; }

	void addvloss(){ INCR(n); }
	void addvtie() { INCR(s); }
	void addvwin() { PLUS(s, 2); }
	void addv(const ExpPair & a){
		if(a.s) PLUS(s, a.s);
		if(a.n) PLUS(n, a.n);
	}

	void addloss(){ n++; }
	void addtie() { s++; }
	void addwin() { s += 2; }
	void add(const ExpPair & a){
		s += a.s;
		n += a.n;
	}

	void addwins(uword num)  { n += num; s += 2*num; }
	void addlosses(uword num){ n += num; }
	ExpPair & operator+=(const ExpPair & a){
		s += a.s;
		n += a.n;
		return *this;
	}
	ExpPair operator + (const ExpPair & a){
		return ExpPair(s + a.s, n + a.n);
	}
	ExpPair & operator*=(uword m){
		s *= m;
		n *= m;
		return *this;
	}
	ExpPair invert(){ //return it from the other player's perspective
		return ExpPair(n*2 - s, n);
	}
};
