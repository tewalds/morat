
#pragma once

// SGF reader/parser, implements the SGF format: http://www.red-bean.com/sgf/

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

#include "fileio.h"
#include "outcome.h"
#include "string.h"

namespace Morat {

template<typename Move>
class SGFPrinter {
	std::ostream & _os;
	bool _root;
	int  _depth;
	bool _indent;

	void print(std::string s) {
		assert(_os.good());
		_os << s;
	}
	void print(std::string key, std::string value) {
		print(key + "[" + value + "]");
	}

public:

	SGFPrinter(std::ostream & os) : _os(os), _root(true), _depth(1) {
		print("(;");
		print("FF", "4");
	}

	void end(){
		print("\n)\n");
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


template<typename Move>
class SGFParser {

	std::istream & _is;
	std::unordered_map<std::string, std::string> _properties;

	void read_node() {
		_properties.clear();
		char key[11], value[1025];
		for(int c = _is.peek(); _is.good() && 'A' <= c && c <= 'Z'; c = _is.peek()) {
			_is.getline(key, 10, '[');
			_is.getline(value, 1024, ']');
			_properties[key] = value;
		}
	}

public:
	SGFParser(std::istream & is) : _is(is) {
		next_child();
	}

	bool next_node() {
		eat_whitespace(_is);
		if(eat_char(_is, ';')){
			read_node();
			return true;
		}
		return false;
	}

	bool has_children() {
		eat_whitespace(_is);
		return (_is.peek() == '(');
	}

	bool next_child() {
		eat_whitespace(_is);
		if(eat_char(_is, '('))
			return next_node();
		return false;
	}

	bool done_child() {
		eat_whitespace(_is);
		return eat_char(_is, ')');
	}

	int size() {
		return from_str<int>(_properties["SZ"]);
	}
	std::string game() {
		return _properties["GM"];
	}

	Move move() {
		if(_properties.count("W"))
			return Move(_properties["W"]);
		if(_properties.count("B"))
			return Move(_properties["B"]);
		return Move();
	}
	std::string comment() {
		return _properties["C"];
	}
};

}; // namespace Morat
