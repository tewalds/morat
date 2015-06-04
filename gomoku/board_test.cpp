
#include "../lib/catch.hpp"
#include "../lib/string.h"

#include "board.h"


using namespace Morat;
using namespace Gomoku;

void test_game(Board b, std::vector<std::string> moves, Outcome outcome) {
	REQUIRE(b.moves_made() == 0);
	Side side = Side::P1;
	int made = 0, remain = 49;
	for(auto s : moves) {
		Outcome expected = (s == moves.back() ? outcome : Outcome::UNKNOWN);
		Move move(s);
		CAPTURE(move);
		CAPTURE(b);
		REQUIRE(b.moves_made() == made);
		REQUIRE(b.moves_remain() == remain);
		REQUIRE(b.valid_move(move));
		REQUIRE(b.to_play() == side);
		REQUIRE(b.test_outcome(move) == expected);
		REQUIRE(b.move(move));
		REQUIRE(b.outcome() == expected);
		side = ~side;
		made++;
		remain--;
	}
	REQUIRE(b.moves_made() == made);
	REQUIRE(b.moves_remain() == (outcome == Outcome::UNKNOWN ? remain : 0));
}
void test_game(Board b, std::string moves, Outcome outcome) {
	test_game(b, explode(moves, " "), outcome);
}

TEST_CASE("Gomoku::Board", "[gomoku][board]") {
	Board b(7);

	SECTION("Basics") {
		REQUIRE(b.get_size() == 7);
		REQUIRE(b.moves_remain() == 49);
	}

	SECTION("sizes") {
		for (int size = Board::min_size; size <= Board::max_size; size++) {
			CAPTURE(size);
			Board b(size);
			REQUIRE(b.move(Move(0, 0)));
			REQUIRE(b.move(Move(size - 1, size - 1)));
		}
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
		std::string invalid[] = {"a0", "a8", "a10", "b8", "e8", "h1", "f0"};
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
		SECTION("x+y") {
			REQUIRE(b.dist(Move("b2"), Move("a1")) == 1);
			REQUIRE(b.dist(Move("b2"), Move("c3")) == 1);
		}
		SECTION("x-y") {
			REQUIRE(b.dist(Move("b2"), Move("a3")) == 1);
			REQUIRE(b.dist(Move("b2"), Move("c1")) == 1);
		}
		SECTION("farther") {
			REQUIRE(b.dist(Move("b2"), Move("d4")) == 2);
			REQUIRE(b.dist(Move("b2"), Move("d3")) == 2);
			REQUIRE(b.dist(Move("b2"), Move("e3")) == 3);
			REQUIRE(b.dist(Move("b2"), Move("d1")) == 2);
		}
	}

	SECTION("win in each direction") {
		test_game(b,    "a1 b1 a2 b2 a3 b3 a4 b4 a5", Outcome::P1);
		test_game(b, "g7 a1 b1 a2 b2 a3 b3 a4 b4 a5", Outcome::P2);

		test_game(b,    "a1 g1 b2 g2 c3 g3 d4 g4 e5", Outcome::P1);
		test_game(b, "g7 a1 g1 b2 g2 c3 g3 d4 g4 e5", Outcome::P2);

		test_game(b,    "a1 g1 b1 g2 c1 g3 d1 g4 e1", Outcome::P1);
		test_game(b, "g7 a1 g1 b1 g2 c1 g3 d1 g4 e1", Outcome::P2);

		test_game(b,    "a5 g1 b4 g2 c3 g3 d2 g5 e1", Outcome::P1);
		test_game(b, "g7 a5 g1 b4 g2 c3 g3 d2 g5 e1", Outcome::P2);

		test_game(b,    "a5 g1 a4 g2 a3 g3 a2 g4 a1", Outcome::P1);
		test_game(b, "g7 a5 g1 a4 g2 a3 g3 a2 g4 a1", Outcome::P2);

		test_game(b,    "e5 g1 d4 g2 c3 g3 b2 g4 a1", Outcome::P1);
		test_game(b, "g7 e5 g1 d4 g2 c3 g3 b2 g4 a1", Outcome::P2);

		test_game(b,    "e1 g1 d1 g2 c1 g3 b1 g4 a1", Outcome::P1);
		test_game(b, "g7 e1 g1 d1 g2 c1 g3 b1 g4 a1", Outcome::P2);

		test_game(b,    "e1 g1 d2 g2 c3 g3 b4 g4 a5", Outcome::P1);
		test_game(b, "g7 e1 g1 d2 g2 c3 g3 b4 g4 a5", Outcome::P2);
	}

	SECTION("win from the middle") {
		test_game(b,    "a1 g1 a5 g2 a2 g3 a4 g5 a3", Outcome::P1);
		test_game(b, "g7 a1 g1 a5 g2 a2 g3 a4 g5 a3", Outcome::P2);
	}
}
