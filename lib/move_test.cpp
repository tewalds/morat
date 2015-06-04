
#include "catch.hpp"

#include "move.h"

namespace Morat {

TEST_CASE("Move", "[move]"){
	REQUIRE(Move() == M_UNKNOWN);
	REQUIRE(Move(M_UNKNOWN) == M_UNKNOWN);
	REQUIRE(Move("unknown") == M_UNKNOWN);
	REQUIRE(Move("a1") == Move(0, 0));
	REQUIRE(Move("c5") == Move(4, 2));
	REQUIRE(Move("a1").to_s() == "a1");
	REQUIRE(Move("c5").to_s() == "c5");
	REQUIRE(Move().to_s() == "unknown");
	REQUIRE(Move("a1") == Move("a1"));
	REQUIRE(Move("a1") <  Move("c5"));
	REQUIRE(Move("a1") <= Move("c5"));
	REQUIRE(Move("a1") != Move("c5"));
	REQUIRE(Move("c5") >  Move("a1"));
	REQUIRE(Move("c5") >= Move("a1"));
	REQUIRE(Move("a1") != M_UNKNOWN);
	REQUIRE((Move("b2") + Move(1, 1)) == Move("c3"));
	REQUIRE((Move("b2") - Move(1, 1)) == Move("a1"));
}

TEST_CASE("MoveValid", "[move]"){
	REQUIRE(MoveValid() == M_UNKNOWN);
	REQUIRE(MoveValid(M_UNKNOWN) == M_UNKNOWN);
	REQUIRE(!MoveValid().on_board());
	REQUIRE(MoveValid("a1", 0).on_board());
	REQUIRE(MoveValid("a1", 0).xy == 0);
	REQUIRE(MoveValid("c5", 42).xy == 42);
	REQUIRE(MoveValid("a1", 0).to_s() == "a1");
	REQUIRE(MoveValid("a1", 0) == MoveValid("a1", 0));
	REQUIRE(MoveValid("a1", 0) <  MoveValid("c5", 42));
	REQUIRE(MoveValid("a1", 0) <= MoveValid("c5", 42));
	REQUIRE(MoveValid("a1", 0) != MoveValid("c5", 42));
	REQUIRE(MoveValid("c5", 42) >  MoveValid("a1", 0));
	REQUIRE(MoveValid("c5", 42) >= MoveValid("a1", 0));
	REQUIRE(MoveValid("a1", 0) != M_UNKNOWN);
	REQUIRE((MoveValid("b2", 11) + Move(1, 1)) == Move("c3"));
	REQUIRE((MoveValid("b2", 11) - Move(1, 1)) == Move("a1"));
}

}; // namespace Morat
