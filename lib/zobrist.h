
#pragma once

//Maintains several zobrist hashes, one for each permutation

#include <stdint.h>

namespace Morat {

extern const uint64_t zobrist_strings[4096];

template<int num>
class Zobrist {
private:
	uint64_t values[num];

public:
	Zobrist(){
		clear();
	}
	void clear() {
		for(int i = 0; i < num; i++)
			values[i] = 0;
	}

	static uint64_t string(int i) {
		return zobrist_strings[i];
	}

	uint64_t test(int permutation, int position) const {
		return values[permutation] ^ zobrist_strings[position];
	}
	void update(int permutation, int position){
		values[permutation] ^= zobrist_strings[position];
	}
	uint64_t get(int permutation) const {
		return values[permutation];
	}

	uint64_t get() const {
		uint64_t m = values[0];
		for(int i = 1; i < num; i++)
			if(m > values[i])
				m = values[i];
		return m;
	}
};

}; // namespace Morat
