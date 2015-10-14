
#include "catch.hpp"

#include "lap_timer.h"
#include "time.h"

namespace Morat {

TEST_CASE("lap", "[lap_timer]") {
	LapTimer t;
	t.start_lap("one", Time(5));
	t.start_lap("two", Time(5.5));
	t.start_lap("three", Time(5.6));
	t.finish_lap(Time(5.755));
	t.start_lap("four", Time(5.8));
	t.finish_lap(Time(5.8557111));
	REQUIRE(t.to_s() == "one:   0.5\ntwo:   0.1\nthree: 0.155\nfour:  0.056\n");
}

TEST_CASE("precision", "[lap_timer]") {
	LapTimer t(6);
	t.start_lap("one", Time(5));
	t.start_lap("two", Time(5.5));
	t.finish_lap(Time(5.8557111));
	REQUIRE(t.to_s() == "one: 0.5\ntwo: 0.355711\n");
}

}; // namespace Morat
