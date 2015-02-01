
#include "../lib/catch.hpp"

#include "agentpns.h"


using namespace Morat;
using namespace Y;

TEST_CASE("Y::AgentPNS::Node::to_s/from_s", "[y][agentpns]") {
	AgentPNS::Node n(Move("a1"));
	auto s = n.to_s();
	AgentPNS::Node k;
	REQUIRE(k.from_s(s));
	REQUIRE(n.to_s() == k.to_s());
}
