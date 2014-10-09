
#pragma once

//Generates random numbers using the XORShift algorithm.
//Read http://xorshift.di.unimi.it/ for more details

#include <stdint.h>

#include "bits.h"
#include "time.h"

namespace Morat {

//generates 32 bit values, has a 32bit period
class XORShift_uint32 {
	uint32_t r;
public:
	XORShift_uint32(uint32_t s = 0) { seed(s); }
	void seed(uint32_t s) { r = mix_bits(s ? s : (uint32_t)Time().in_usec()); }
	uint32_t operator()() { return rand(); }
protected:
	uint32_t rand(){
		r ^= (r << 13);
		r ^= (r >> 17);
		r ^= (r <<  5);
		return r * 1597334677;
	}
};

//generates 64 bit values, has a 64bit period
class XORShift_uint64 {
	uint64_t r;
public:
	XORShift_uint64(uint64_t s = 0) { seed(s); }
	void seed(uint64_t s) { r = mix_bits(s ? s : Time().in_usec()); }
	uint64_t operator()() { return rand(); }
protected:
	uint64_t rand(){
		r ^= r >> 12; // a
		r ^= r << 25; // b
		r ^= r >> 27; // c
		return r * 2685821657736338717LL;
	}
};

//generates 64 bit values, has a 128bit period
class XORShift_uint128 {
	uint64_t r[2];
public:
	XORShift_uint128(uint64_t s = 0) { seed(s); }
	void seed(uint64_t s) {
		r[0] = mix_bits(s ? s : Time().in_usec());
		r[1] = mix_bits(r[0]);
	}
	uint64_t operator()() { return rand(); }
protected:
	uint64_t rand(){
		uint64_t r1 = r[0];
		const uint64_t r0 = r[1];
		r[0] = r0;
		r1 ^= r1 << 23;  // a
		r[1] = r1 ^ r0 ^ (r1 >> 17) ^ (r0 >> 26);  // b, c
		return r[1] + r0;
	}
};

// generates floating point numbers in the half-open interval [0, 1)
class XORShift_float : XORShift_uint32 {
public:
	XORShift_float(uint32_t seed = 0) : XORShift_uint32(seed) {}
	float operator()() { return static_cast<float>(rand()) * (1.f / 4294967296.f ); } // divide by 2^32
};

// generates double floating point numbers in the half-open interval [0, 1)
class XORShift_double : XORShift_uint64 {
public:
	XORShift_double(uint64_t seed = 0) : XORShift_uint64(seed) {}
	double operator()() { return static_cast<double>(rand()) * (1. / 18446744073709551616.); } // divide by 2^64
};

}; // namespace Morat
