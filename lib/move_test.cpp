
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
	// REQUIRE(Move("c5") + Move(1,1) == Move("d6"));
	// REQUIRE(Move("c5") - Move(1,1) == Move("b4"));
}

}; // namespace Morat
