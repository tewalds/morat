
#pragma once

#include <cstdio>
#include <istream>
#include <string>

int fpeek(FILE * fd);
void eat_whitespace(FILE * fd);
void eat_whitespace(std::istream & is);
bool eat_char(FILE * fd, int expect);
bool eat_char(std::istream & is, int expect);
std::string read_until(FILE * fd, char until, bool include = false);
