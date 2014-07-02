
#include <cassert>

#include "fileio.h"

namespace Morat {
using namespace std;

int fpeek(FILE * fd){
	int c = fgetc(fd);
	ungetc(c, fd);
	return c;
}
void eat_whitespace(FILE * fd){
	int c = fgetc(fd);
	while(c == ' ' || c == '\n' || c == '\t')
		c = fgetc(fd);
	ungetc(c, fd);
}
void eat_whitespace(std::istream & is){
	int c = is.peek();
	while(c == ' ' || c == '\n' || c == '\t'){
		is.get();
		c = is.peek();
	}
}
bool eat_char(FILE * fd, int expect){
	int c = fgetc(fd);
	if (c == expect)
		return true;
	ungetc(c, fd);
	return false;
}
bool eat_char(std::istream & is, int expect){
	int c = is.peek();
	if (c == expect){
		is.get();
		return true;
	}
	return false;
}
string read_until(FILE * fd, char until, bool include){
	string ret;
	char c = fgetc(fd);
	while(c != until){
		ret += c;
		c = fgetc(fd);
	}
	if(!include)
		ungetc(c, fd);
	return ret;
}

};
