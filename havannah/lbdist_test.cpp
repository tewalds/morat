
#include "../lib/catch.hpp"
#include "../lib/string.h"

#include "board.h"
#include "lbdist.h"


using namespace Morat;
using namespace Havannah;

TEST_CASE("Havannah::LBDists", "[havannah][LBDists]") {
	Board b(4);
	LBDists d;

	SECTION("Basics") {
		auto moves = explode("a1 d2 a2 e3 a3 f4", " ");
		for(auto m : moves) {
			REQUIRE(b.move(m));
		}
		d.run(&b);

		CAPTURE("\n" + d.to_s(Side::P1));
		CAPTURE("\n" + d.to_s(Side::P2));

		REQUIRE(d.get(Move("a4"), Side::P1) == 1);
		REQUIRE(d.get(Move("a4"), Side::P2) == 4);

		REQUIRE(d.get(Move("d1"), Side::P1) == 3);
		REQUIRE(d.get(Move("d1"), Side::P2) == 2);

		REQUIRE(d.get(Move("g4"), Side::P1) == 4);
		REQUIRE(d.get(Move("g4"), Side::P2) == 2);

		REQUIRE(d.get(Move("e2"), Side::P1) == 4);
		REQUIRE(d.get(Move("e2"), Side::P2) == 3);

		REQUIRE(d.get(Move("g7"), Side::P1) == 4);
		REQUIRE(d.get(Move("g7"), Side::P2) == 4);
	}

	SECTION("Dead area") {
		auto moves = explode("b1 e4 b2 d5 a2", " ");
		for(auto m : moves) {
			REQUIRE(b.move(m));
		}
		d.run(&b);

		CAPTURE("\n" + d.to_s(Side::P1));
		CAPTURE("\n" + d.to_s(Side::P2));

		REQUIRE(d.get(Move("a1"), Side::P1) == 3);
		REQUIRE(d.get(Move("a1"), Side::P2) > 100);
	}

	SECTION("VCs") {
		auto moves = explode("c1 f4 c2 e4 b3 d4 a3 g4", " ");
		for(auto m : moves) {
			REQUIRE(b.move(m));
		}

		d.run(&b, true);

		CAPTURE("\n" + d.to_s(Side::P1));
		CAPTURE("\n" + d.to_s(Side::P2));
		REQUIRE(d.get(Move("a1"), Side::P1) == 3);
		REQUIRE(d.get(Move("a1"), Side::P2) == 3);

		d.run(&b, false);

		CAPTURE("\n" + d.to_s(Side::P1));
		CAPTURE("\n" + d.to_s(Side::P2));
		REQUIRE(d.get(Move("a1"), Side::P1) == 3);
		REQUIRE(d.get(Move("a1"), Side::P2) > 100);
	}
}
