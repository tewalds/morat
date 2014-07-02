
#pragma once

#include <cstdio>
#include <string>

namespace Morat {

inline void logerr(std::string str){
	fprintf(stderr, "%s", str.c_str());
}

}; // namespace Morat
