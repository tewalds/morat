
#include "catch.hpp"

#include "string.h"

using namespace std;

TEST_CASE("to_str", "[string]"){
	REQUIRE("1" == to_str(1));
	REQUIRE("1.5" == to_str(1.5));
	REQUIRE("3.14" == to_str(3.14159, 2));
}

TEST_CASE("from_str", "[string]"){
	REQUIRE(1 == from_str<int>("1"));
	REQUIRE(1.5 == from_str<float>("1.5"));
}

TEST_CASE("trim", "[string]"){
	string s = "   hello world   \n";

	SECTION("trim") {
		trim(s);
		REQUIRE(s == "hello world");
	}

	SECTION("ltrim") {
		ltrim(s);
		REQUIRE(s == "hello world   \n");
	}

	SECTION("rtrim") {
		rtrim(s);
		REQUIRE(s == "   hello world");
	}
}

TEST_CASE("explode/explode", "[string]"){
	string s = "hello cruel world";

	SECTION("explode"){
		auto parts = explode(s, " ");
		REQUIRE(parts.size() == 3);
		REQUIRE(parts[0] == "hello");
		REQUIRE(parts[1] == "cruel");
		REQUIRE(parts[2] == "world");
	}

	SECTION("explode length 1"){
		auto parts = explode(s, " ", 1);
		REQUIRE(parts.size() == 1);
		REQUIRE(parts[0] == s);
	}

	SECTION("explode length 2"){
		auto parts = explode(s, " ", 2);
		REQUIRE(parts.size() == 2);
		REQUIRE(parts[0] == "hello");
		REQUIRE(parts[1] == "cruel world");
	}

	SECTION("implode"){
		auto parts = explode(s, " ");
		auto r = implode(parts, " ");
		REQUIRE(s == r);
	}
}

TEST_CASE("parse_dict", "[string]"){
	string s = "key: value, key2: val2";
	auto d = parse_dict(s, ", ", ": ");
	REQUIRE(d.size() == 2);
	REQUIRE(d["key"] == "value");
	REQUIRE(d["key2"] == "val2");
}
