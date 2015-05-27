
#include "catch.hpp"
#include "timecontrol.h"


namespace Morat {

TEST_CASE("percent", "[timecontrol]") {
	TimeControl tc;
	tc.method = TimeControl::PERCENT;
	tc.param = 10;
	tc.game = 100;
	tc.move = 0;
	tc.new_game();

	REQUIRE(tc.get_time(0, 10, 10) == 10.0);
	tc.use(10);
	REQUIRE(tc.get_time(0, 10, 10) == 9.0);
}

TEST_CASE("even", "[timecontrol]") {
	TimeControl tc;
	tc.method = TimeControl::EVEN;
	tc.param = 2;
	tc.game = 100;
	tc.move = 0;
	tc.new_game();

	REQUIRE(tc.get_time(0, 10, 5) == 40.0);  // 100/10*2*2
	tc.use(20);
	REQUIRE(tc.get_time(2, 8, 5) == 40.0);
}

TEST_CASE("stats", "[timecontrol]") {
	TimeControl tc;
	tc.method = TimeControl::STATS;
	tc.param = 2;
	tc.game = 100;
	tc.move = 0;
	tc.new_game();

	REQUIRE(tc.get_time(0, 10, 0) == 40.0);  // falling back to even: 100/10*2*2
	REQUIRE(tc.get_time(0, 10, 5) == 80.0);  // 100/5*2*2
	tc.use(40);
	REQUIRE(tc.get_time(2, 8, 5) == 48.0);
}

TEST_CASE("move", "[timecontrol]") {
	TimeControl tc;
	tc.method = TimeControl::PERCENT;
	tc.param = 10;
	tc.game = 100;
	tc.move = 5;
	tc.new_game();

	tc.flexible = false;

	REQUIRE(tc.get_time(0, 10, 5) == 15);

	REQUIRE(tc.remain == 100);
	tc.use(5);
	REQUIRE(tc.remain == 100);
	tc.use(2);
	REQUIRE(tc.remain == 100);
	tc.use(10);
	REQUIRE(tc.remain == 95);

	tc.flexible = true;

	tc.use(2);
	REQUIRE(tc.remain == 98);
	tc.use(10);
	REQUIRE(tc.remain == 93);

}

}; // namespace Morat
