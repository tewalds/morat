
#include "../lib/catch.hpp"
#include "../lib/string.h"

#include "board.h"


using namespace Morat;
using namespace Hex;

void test_game(Board b, std::vector<std::string> moves, Outcome outcome) {
	REQUIRE(b.num_moves() == 0);
	Side side = Side::P1;
	for(auto s : moves) {
		Outcome expected = (s == moves.back() ? outcome : Outcome::UNKNOWN);
		Move move(s);
		CAPTURE(move);
		CAPTURE(b);
		REQUIRE(b.valid_move(move));
		REQUIRE(b.toplay() == side);
		REQUIRE(b.test_outcome(move) == expected);
		REQUIRE(b.move(move));
		REQUIRE(b.won() == expected);
		side = ~side;
	}
}
void test_game(Board b, std::string moves, Outcome outcome) {
	test_game(b, explode(moves, " "), outcome);
}

TEST_CASE("Hex::Board", "[hex][board]") {
	Board b(7);

	SECTION("Basics") {
		REQUIRE(b.get_size() == 7);
		REQUIRE(b.movesremain() == 49);
	}

	SECTION("valid moves") {
		std::string valid[] = {"A1", "D4",
		"a1", "a2", "a3", "a4", "a5", "a6", "a7",
		   "b1", "b2", "b3", "b4", "b5", "b6", "b7",
		      "c1", "c2", "c3", "c4", "c5", "c6", "c7",
		         "d1", "d2", "d3", "d4", "d5", "d6", "d7",
		            "e1", "e2", "e3", "e4", "e5", "e6", "e7",
		               "f1", "f2", "f3", "f4", "f5", "f6", "f7",
		                  "g1", "g2", "g3", "g4", "g5", "g6", "g7",
		};
		for(auto m : valid){
			REQUIRE(b.onboard(m));
			REQUIRE(b.valid_move(m));
		}
	}

	SECTION("invalid moves") {
		std::string invalid[] = {"a0", "a8", "a10", "b8", "c8", "e0", "e8", "f8", "f0", "h1", "f0"};
		for(auto m : invalid){
			REQUIRE_FALSE(b.onboard(m));
			REQUIRE_FALSE(b.valid_move(m));
		}
	}

	SECTION("duplicate moves") {
		Move m("a1");
		REQUIRE(b.valid_move(m));
		REQUIRE(b.move(m));
		REQUIRE_FALSE(b.valid_move(m));
		REQUIRE_FALSE(b.move(m));
	}

	SECTION("move distance") {
		SECTION("x") {
			REQUIRE(b.dist(Move("b2"), Move("b1")) == 1);
			REQUIRE(b.dist(Move("b2"), Move("b3")) == 1);
		}
		SECTION("y") {
			REQUIRE(b.dist(Move("b2"), Move("a2")) == 1);
			REQUIRE(b.dist(Move("b2"), Move("c2")) == 1);
		}
		SECTION("z") {
			REQUIRE(b.dist(Move("b2"), Move("a3")) == 1);
			REQUIRE(b.dist(Move("b2"), Move("c1")) == 1);
		}
		SECTION("farther") {
			REQUIRE(b.dist(Move("b2"), Move("a1")) == 2);
			REQUIRE(b.dist(Move("b2"), Move("c3")) == 2);
			REQUIRE(b.dist(Move("b2"), Move("d4")) == 4);
			REQUIRE(b.dist(Move("b2"), Move("d3")) == 3);
			REQUIRE(b.dist(Move("b2"), Move("d1")) == 2);
			REQUIRE(b.dist(Move("b2"), Move("e3")) == 4);
		}
	}

	SECTION("Unknown_1") {
		test_game(b, {      "a1", "b1", "a2", "b2", "a3", "b3", "a4"}, Outcome::UNKNOWN);
		test_game(b, {"d4", "a1", "b1", "a2", "b2", "a3", "b3", "a4"}, Outcome::UNKNOWN);
	}

	SECTION("Unknown_2") {
		test_game(b, {      "b1", "c1", "b2", "c2", "b3", "c3", "b4", "c4", "b5", "c5", "a2"}, Outcome::UNKNOWN);
		test_game(b, {"d4", "b1", "c1", "b2", "c2", "b3", "c3", "b4", "c4", "b5", "c5", "a2"}, Outcome::UNKNOWN);
	}

	SECTION("Unknown_3") {
		test_game(b, {      "b2", "f3", "b3", "f4", "c2", "f5", "c4", "f6", "d3", "f7", "d4"}, Outcome::UNKNOWN);
		test_game(b, {"d7", "b2", "f3", "b3", "f4", "c2", "f5", "c4", "f6", "d3", "f7", "d4"}, Outcome::UNKNOWN);

		test_game(b, {      "b2", "f3", "b3", "f4", "c2", "f5", "c4", "f6", "d3", "f7", "c3", "e6", "d4"}, Outcome::UNKNOWN);
		test_game(b, {"d7", "b2", "f3", "b3", "f4", "c2", "f5", "c4", "f6", "d3", "f7", "c3", "e6", "d4"}, Outcome::UNKNOWN);
	}
	
	SECTION("White Connects") {
		test_game(b, 
			"c2 c5 e4 d6 c6 d5 f5 e3 d4 d3 b4 d1 a6 c1 d2 b3 c3 e1 f2 a7 b6 b7 c7 a5 f3 e7 g6 g7 f7 g4 f1 g2 b5 e2 c4",
			 Outcome::P1);
	}
	
	SECTION("Black Connects") {
		test_game(b, 
			"d1 d4 e4 e3 g2 f3 g3 f5 g5 g4 a6 b5 f4 d6 e5 g1 a5 b3 c5 c3 a3 b6 g7 a7 f7 f2 c4 d3 a4 b4",
			Outcome::P2);
	}
}
