
#include <cmath>

#include "string.h"
#include "types.h"

using namespace std;

string to_str(double a, int prec){
	double p = pow(10.0, prec);
	a = round(a*p)/p;

	stringstream out;
	out << a;
	return out.str();
}

void trim(string & str){
	const char * space = " \t\r\n";
    str.erase(0,  str.find_first_not_of(space));
    str.erase(1 + str.find_last_not_of(space));
}

void ltrim(string & str){
	const char * space = " \t\r\n";
    str.erase(0,  str.find_first_not_of(space));
}

void rtrim(string & str){
	const char * space = " \t\r\n";
    str.erase(1 + str.find_last_not_of(space));
}

vecstr explode(const string & str, const string & sep, int count){
	vecstr ret;
	string::size_type old = 0, pos = 0;
	while((pos = str.find(sep, old)) != string::npos && --count != 0){
		ret.push_back(str.substr(old, pos - old));
		old = pos + sep.length();
	}
	ret.push_back(str.substr(old));
	return ret;
}

string implode(const vecstr & vec, const string & sep){
	string ret;
	if(vec.size() == 0)
		return ret;
	ret += vec[0];
	for(unsigned int i = 1; i < vec.size(); i++){
		ret += sep;
		ret += vec[i];
	}
	return ret;
}

dictstr parse_dict(const std::string & str, const std::string & sep1, const std::string & sep2) {
	dictstr ret;
	auto parts = explode(str, sep1);
	for(auto & p : parts){
		auto kv = explode(p, sep2, 2);
		ret[kv[0]] = (kv.size() == 2 ? kv[1] : "");
	}
	return ret;
}
