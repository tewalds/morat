
#pragma once

#include <stdint.h>

namespace Morat {

#define trailing_zeros(n) __builtin_ctz(n)

// https://code.google.com/p/smhasher/wiki/MurmurHash3
inline uint32_t mix_bits(uint32_t h){
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

// https://code.google.com/p/smhasher/wiki/MurmurHash3
inline uint64_t mix_bits(uint64_t h){
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdull;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53ull;
	h ^= h >> 33;
	return h;
}

//round a number up to the nearest power of 2
template <typename NumType>
NumType roundup(NumType v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

}; // namespace Morat
