
#pragma once

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "time.h"

namespace Morat {

// A basic lap timer, mainly useful for benchmarking:
// LapTimer t;
// t.start_lap('version1');
// version1();
// t.start_lap('version2');
// version2();
// cout << t;
class LapTimer {
	std::vector<std::pair<std::string, double>> times;
	bool running;
	int precision;
	std::string cur_lap_name;
	Time cur_lap_start;

public:

	LapTimer(int precision=3) : running(false), precision(precision) {}

	void start_lap(std::string name);
	void start_lap(std::string name, const Time& t);  // useful for unit tests
	void finish_lap();
	void finish_lap(const Time& t);  // useful for unit tests

	// These output the results and implicitly finish the running lap if there is one.
	std::string to_s();
	friend std::ostream& operator<< (std::ostream& out, LapTimer& timer);
};

}; // namespace Morat
