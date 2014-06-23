
#include <cassert>

#include "fileio.h"

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
bool eat_char(FILE * fd, int expect){
	int c = fgetc(fd);
	if (c == expect)
		return true;
	ungetc(c, fd);
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
