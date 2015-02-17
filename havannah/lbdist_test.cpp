
#include "../lib/catch.hpp"

#include "board.h"
#include "lbdist.h"


using namespace Morat;
using namespace Havannah;

TEST_CASE("Havannah::LBDists", "[havannah][LBDists]") {
	Board b(4);
	LBDists d;

	SECTION("Basics") {
		std::string moves[] = {"a1", "b1", "a2", "b2", "a3", "b3"};
		for(auto m : moves){
			REQUIRE(b.move(m));
		}
		d.run(&b);

		CAPTURE("\n" + b.to_s(true, std::bind(&LBDists::get_s, &d, std::placeholders::_1, Side::P1)));

		REQUIRE(d.get(Move("a4"), Side::P1) == 1);
		REQUIRE(d.get(Move("a4"), Side::P1) == 1);

		REQUIRE(d.get(Move("c1"), Side::P1) == 5);
	}
}
