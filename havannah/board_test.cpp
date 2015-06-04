
#include "../lib/catch.hpp"

#include "board.h"


using namespace Morat;
using namespace Havannah;

void test_game(Board b, std::vector<std::string> moves, Outcome outcome) {
	REQUIRE(b.moves_made() == 0);
	Side side = Side::P1;
	int made = 0, remain = 37;
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

TEST_CASE("Havannah::Board", "[havannah][board]") {
	Board b(4);

	SECTION("Basics") {
		REQUIRE(b.get_size() == 4);
		REQUIRE(b.get_size_d() == 7);
		REQUIRE(b.moves_remain() == 37);
	}

	SECTION("valid moves") {
		std::string valid[] = {"A1", "D4",
		         "a1", "a2", "a3", "a4",
		      "b1", "b2", "b3", "b4", "b5",
		   "c1", "c2", "c3", "c4", "c5", "c6",
		"d1", "d2", "d3", "d4", "d5", "d6", "d7",
		   "e2", "e3", "e4", "e5", "e6", "e7",
		      "f3", "f4", "f5", "f6", "f7",
		         "g4", "g5", "g6", "g7",
		};
		for(auto m : valid){
			REQUIRE(b.on_board(m));
			REQUIRE(b.valid_move(m));
		}
	}

	SECTION("invalid moves") {
		std::string invalid[] = {"a0", "a5", "a10", "b6", "c7", "e1", "e8", "f1", "f2", "h1", "f0"};
		for(auto m : invalid){
			REQUIRE_FALSE(b.on_board(m));
			REQUIRE_FALSE(b.valid_move(m));
		}
	}

	SECTION("edges") {
		std::string edges[] = {"a2", "a3", "b5", "c6", "e7", "f7", "g6", "g5", "f3", "e2", "c1", "b1"};
		std::string corners[] = {"a1", "a4", "d7", "g7", "g4", "d1"};
		std::string middle[] = {
		      "b2", "b3", "b4",
		   "c2", "c3", "c4", "c5",
		"d2", "d3", "d4", "d5", "d6",
		   "e3", "e4", "e5", "e6",
		      "f4", "f5", "f6",
		};

		for(auto m : edges){
			auto * c = b.cell(Move(m));
			REQUIRE(c->numedges() == 1);
			REQUIRE(c->numcorners() == 0);
		}

		for(auto m : corners){
			auto * c = b.cell(Move(m));
			REQUIRE(c->numedges() == 0);
			REQUIRE(c->numcorners() == 1);
		}

		for(auto m : middle){
			auto * c = b.cell(Move(m));
			REQUIRE(c->numedges() == 0);
			REQUIRE(c->numcorners() == 0);
		}
	}

	SECTION("duplicate moves") {
		Move m("a1");
		REQUIRE(b.valid_move(m));
		REQUIRE(b.move(m));
		REQUIRE_FALSE(b.valid_move(m));
		REQUIRE_FALSE(b.move(m));
	}

	SECTION("num moves, moves remain") {
		std::string moves[] = {
		         "a1", "a2", "a3", "a4",
		      "b1", "b2", "b3", "b4", "b5",
		   "c1", "c2", "c3", "c4", "c5", "c6",
		"d1", "d2", "d3", "d4", "d5", "d6", "d7",
		   "e2", "e3", "e4", "e5", "e6", "e7",
		      "f3", "f4", "f5", "f6", "f7",
		         "g4", "g5", "g6", "g7",
		};
		int made = 0, remain = 37;
		for(auto m : moves) {
			REQUIRE(b.moves_made() == made);
			REQUIRE(b.moves_remain() == remain);
			b.move(Move(m));
			made++;
			remain--;
		}
		REQUIRE(b.moves_made() == made);
		REQUIRE(b.moves_remain() == remain);
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
			REQUIRE(b.dist(Move("b2"), Move("a1")) == 1);
			REQUIRE(b.dist(Move("b2"), Move("c3")) == 1);
		}
		SECTION("farther") {
			REQUIRE(b.dist(Move("b2"), Move("a3")) == 2);
			REQUIRE(b.dist(Move("b2"), Move("c1")) == 2);
			REQUIRE(b.dist(Move("b2"), Move("d4")) == 2);
			REQUIRE(b.dist(Move("b2"), Move("d3")) == 2);
			REQUIRE(b.dist(Move("b2"), Move("e3")) == 3);
			REQUIRE(b.dist(Move("b2"), Move("d1")) == 3);
		}
	}

	SECTION("bridge") {
		test_game(b, {      "a1", "b1", "a2", "b2", "a3", "b3", "a4"}, Outcome::P1);
		test_game(b, {"d4", "a1", "b1", "a2", "b2", "a3", "b3", "a4"}, Outcome::P2);
	}

	SECTION("fork") {
		test_game(b, {      "b1", "c1", "b2", "c2", "b3", "c3", "b4", "c4", "b5", "c5", "a2"}, Outcome::P1);
		test_game(b, {"d4", "b1", "c1", "b2", "c2", "b3", "c3", "b4", "c4", "b5", "c5", "a2"}, Outcome::P2);
	}

	SECTION("ring") {
		test_game(b, {      "b2", "f3", "b3", "f4", "c2", "f5", "c4", "f6", "d3", "f7", "d4"}, Outcome::P1);
		test_game(b, {"d7", "b2", "f3", "b3", "f4", "c2", "f5", "c4", "f6", "d3", "f7", "d4"}, Outcome::P2);
	}

	SECTION("filled ring") {
		test_game(b, {      "b2", "f3", "b3", "f4", "c2", "f5", "c4", "f6", "d3", "f7", "c3", "e6", "d4"}, Outcome::P1);
		test_game(b, {"d7", "b2", "f3", "b3", "f4", "c2", "f5", "c4", "f6", "d3", "f7", "c3", "e6", "d4"}, Outcome::P2);
	}

	SECTION("draw") {
		test_game(b, {
		         "a1", "a2", "a3", "a4",
		      "b1", "b2", "b3", "b4", "b5",
		   "c1", "c2", "c3", "c4", "c5", "c6",
		"d1", "d2", "d3", "d4", "d5", "d6", "d7",
		   "e2", "e3", "e4", "e5", "e6", "e7",
		      "f3", "f4", "f5", "f6", "f7",
		         "g4", "g5", "g6", "g7",
		}, Outcome::DRAW);
	}
}
