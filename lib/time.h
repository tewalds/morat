
#pragma once

#include <time.h>
#include <stdint.h>
#include <sys/time.h>

namespace Morat {

class Time {
	double t;
public:
	Time(){
		struct timeval time;
		gettimeofday(&time, NULL);
		t = time.tv_sec + (double)time.tv_usec/1000000;
	}
	Time(double a) : t(a) { }
	Time(const struct timeval & time){
		t = time.tv_sec + (double)time.tv_usec/1000000;
	}

	double   to_f()    const { return t; }
	uint64_t to_i()    const { return (uint64_t)t; }
	uint64_t in_msec() const { return (uint64_t)(t*1000); }
	uint64_t in_usec() const { return (uint64_t)(t*1000000); }

	Time   operator +  (double a)       const { return Time(t+a); }
	Time & operator += (double a)             { t += a; return *this; }
	double operator -  (const Time & a) const { return t - a.t; }
	Time   operator -  (double a)       const { return Time(t-a); }
	Time & operator -= (double a)             { t -= a; return *this; }

	bool operator <  (const Time & a) const { return t <  a.t; }
	bool operator <= (const Time & a) const { return t <= a.t; }
	bool operator >  (const Time & a) const { return t >  a.t; }
	bool operator >= (const Time & a) const { return t >= a.t; }
	bool operator == (const Time & a) const { return t == a.t; }
	bool operator != (const Time & a) const { return t != a.t; }
};

}; // namespace Morat
