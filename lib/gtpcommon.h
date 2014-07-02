
#pragma once

#include "gtpbase.h"
#include "timecontrol.h"

namespace Morat {

class GTPCommon : public GTPBase {

protected:
	TimeControl time_control;

public:

	GTPCommon(FILE * i, FILE * o) : GTPBase(i, o) {
		newcallback("echo",     bind(&GTPCommon::gtp_echo,   this, _1), "Return the arguments as the response");
		newcallback("time",     bind(&GTPCommon::gtp_time,   this, _1), "Set the time limits and the algorithm for per game time");
	}

	GTPResponse gtp_echo(vecstr args) const;
	GTPResponse gtp_time(vecstr args);
	std::string won_str(int outcome) const;
};

}; // namespace Morat
