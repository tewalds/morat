
#include "catch.hpp"

#include "outcome.h"

TEST_CASE("Side and Outcome", "[side][outcome]"){
	REQUIRE(~Side::P1    == Side::P2);
	REQUIRE(~Side::P2    == Side::P1);

	REQUIRE(+Side::NONE == Outcome::DRAW); // neither side wins => draw
	REQUIRE(+Side::P1   == Outcome::P1);
	REQUIRE(+Side::P2   == Outcome::P2);
	REQUIRE(+Side::BOTH == Outcome::DRAW2); // both sides win => draw

	REQUIRE(-Side::NONE  == Outcome::UNKNOWN); // neither side is known to not win => unknown
	REQUIRE(-Side::P1    == Outcome::P2_DRAW); // p1 can't win => p2 can win or draw
	REQUIRE(-Side::P2    == Outcome::P1_DRAW); // p2 can't win => p1 can win or draw
	REQUIRE(-Side::BOTH  == Outcome::DRAW);    // both sides can't win => no one can win => draw
	REQUIRE(-Outcome::P1 == Outcome::P2_DRAW);
	REQUIRE(-Outcome::P2 == Outcome::P1_DRAW);

	REQUIRE(-~Side::P1   == Outcome::P1_DRAW);
	REQUIRE(-~Side::P2   == Outcome::P2_DRAW);

	// invalid! wrong order! ~ and - are not commutative
	//: REQUIRE(~-Side::P1   == Outcome::P1_DRAW);

	REQUIRE(Side::P1 == Outcome::P1);
	REQUIRE(Side::P2 == Outcome::P2);
	REQUIRE(Outcome::P1 == Side::P1);
	REQUIRE(Outcome::P2 == Side::P2);
	REQUIRE(Side::P1 != Outcome::P2);
	REQUIRE(Side::P2 != Outcome::P1);
	REQUIRE(Outcome::P1 != Side::P2);
	REQUIRE(Outcome::P2 != Side::P1);

	REQUIRE((Side::P1    | Side::P2)    == Side::BOTH);
	REQUIRE((Outcome::P1 | Outcome::P2) == Outcome::DRAW2);
}
