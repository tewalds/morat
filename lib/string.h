
#pragma once

//A few basic string operation helpers

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

namespace Morat {

typedef std::vector<std::string> vecstr;
typedef std::unordered_map<std::string, std::string> dictstr;

template <class T> std::string to_str(T a){
	std::stringstream out;
	out << a;
	return out.str();
}

std::string to_str(double a, int prec);

std::string to_str_hex(uint64_t a);

template <class T> T from_str(const std::string & str){
	std::istringstream sin(str);
	T ret;
	sin >> ret;
	return ret;
}

template <class T> void from_str(T& ret, const std::string & str) {
	std::istringstream sin(str);
	sin >> ret;
}


void trim(std::string & str);
void ltrim(std::string & str);
void rtrim(std::string & str);

vecstr explode(const std::string & str, const std::string & sep, int count=0);
std::string implode(const vecstr & vec, const std::string & sep);
dictstr parse_dict(const std::string & str, const std::string & sep1, const std::string & sep2);

}; // namespace Morat
