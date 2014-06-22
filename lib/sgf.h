
#pragma once

#include <string>
#include <cassert>

#include "fileio.h"
#include "outcome.h"
#include "string.h"

template<typename Move>
class SGFPrinter {
	FILE * _fd;
	bool   _root;
	int    _depth;
	bool   _indent;

	void print(std::string s) {
		assert(_fd);
		fprintf(_fd, "%s", s.c_str());
	}
	void print(std::string key, std::string value) {
		print(key + "[" + value + "]");
	}

public:

	SGFPrinter(FILE * fd) : _fd(fd), _root(true), _depth(1) {
		print("(;");
		print("FF", "4");
	}

	void end(){
		print("\n)\n");
		_fd = NULL;
	}

	void size(int s) {
		assert(_root);
		print("SZ", to_str(s));
	}
	void game(std::string name) {
		assert(_root);
		print("GM", name);
	}
	void program(std::string name, std::string version) {
		assert(_root);
		print("AP", name + ":" + version);
	}

	void end_root() {
		assert(_root);
		_root = false;
		print("\n ");
	}

	void child_start() {
		assert(!_root);
		assert(_depth >= 1);
		print("\n" + std::string(_depth, ' ') + "(");
		_depth++;
		_indent = false;
	}
	void child_end() {
		assert(!_root);
		assert(_depth >= 1);
		_depth--;
		if(_indent)
			print("\n" + std::string(_depth, ' '));
		print(")");
		_indent = true;
	}
	void move(Side s, Move m) {
		assert(!_root);
		print(";");
		print((s == Side::P1 ? "W" : "B"), m.to_s());
	}
	void comment(std::string c) {
		assert(!_root);
		print("C", c);
	}
};

/*
template<typename Move>
class SGFParser {


public:

	int size();
	std::string game();

	Move move();
	std::string comment();

	SGFParser begin();
	SGFParser end();

};

*/
