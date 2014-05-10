
#pragma once

#include "gtpbase.h"
#include "timecontrol.h"

class GTPCommon : public GTPBase {

protected:
	TimeControl time_control;

public:

	GTPCommon(FILE * i, FILE * o) : GTPBase(i, o) {
		newcallback("echo",     bind(&GTPCommon::gtp_echo,   this, _1), "Return the arguments as the response");
		newcallback("time",     bind(&GTPCommon::gtp_time,   this, _1), "Set the time limits and the algorithm for per game time");
	}

	GTPResponse gtp_echo(vecstr args);
	GTPResponse gtp_time(vecstr args);
};
