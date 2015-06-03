
#include "base_board.h"

namespace Morat {

std::string BaseBoard::hash_str() const {
	static const char hexlookup[] = "0123456789abcdef";
	char buf[19] = "0x";
	hash_t val = hash();
	for(int i = 15; i >= 0; i--){
		buf[i+2] = hexlookup[val & 15];
		val >>= 4;
	}
	buf[18] = '\0';
	return (char *)buf;
}

std::string empty(Move m) { return "."; }

std::string BaseBoard::to_s(bool color) const {
	return to_s(color, empty);
}

}; // namespace Morat
