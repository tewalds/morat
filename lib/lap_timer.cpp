
#include <algorithm>

#include "lap_timer.h"
#include "string.h"

namespace Morat {

void LapTimer::start_lap(std::string name) { start_lap(name, Time()); }
void LapTimer::start_lap(std::string name, const Time& t) {
	finish_lap(t);
	cur_lap_start = t;
	cur_lap_name = name;
	running = true;
}

void LapTimer::finish_lap() { finish_lap(Time()); }
void LapTimer::finish_lap(const Time& t) {
	if (running) {
		times.push_back(make_pair(cur_lap_name, t - cur_lap_start));
		running = false;
	}
}

std::string LapTimer::to_s() {
	return to_str(*this);
}

std::ostream& operator<< (std::ostream& out, LapTimer& timer) {
	timer.finish_lap();

	// Find the longest name so we can align them
	unsigned long longest = 0;
	for (const auto& p : timer.times) {
		longest = std::max(longest, p.first.length());
	}

	for (const auto& p : timer.times) {
		out << p.first << ": " << std::string(longest - p.first.length(), ' ')
		    << to_str(p.second, timer.precision) << "\n";
	}
	return out;
}

}; // namespace Morat
