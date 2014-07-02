
#pragma once

#include <stdint.h>

namespace Morat {

#define trailing_zeros(n) __builtin_ctz(n)

inline uint32_t mix_bits(uint32_t h){
	h ^= (h << 13);
	h ^= (h >> 17);
	h ^= (h <<  5);
	return h;
}

inline uint64_t mix_bits(uint64_t h){
	h ^= (h >> 17);
	h ^= (h << 31);
	h ^= (h >>  8);
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
