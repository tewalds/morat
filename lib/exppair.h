
#pragma once

#include "string.h"
#include "thread.h"
#include "types.h"

class ExpPair {
	uword s, n;
	ExpPair(uword S, uword N) : s(S), n(N) { }
public:
	ExpPair() : s(0), n(0) { }
	float avg() const { return (n ? 0.5f*s/n : 0); }
	uword num() const { return n; }
	uword sum() const { return s/2; }

	std::string to_s() const {
		return to_str(avg(), 3) + "/" + to_str(num());
	}

	ExpPair(std::string str) {
		auto parts = explode(str, "/");
		assert(parts.size() == 2);
		n = from_str<uword>(parts[1]);
		s = 2.0*from_str<float>(parts[0])*n;
	}

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
