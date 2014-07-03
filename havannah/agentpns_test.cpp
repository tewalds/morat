
#include "../lib/catch.hpp"

#include "agentpns.h"

using namespace Morat;
using namespace Havannah;

TEST_CASE("Havannah::AgentPNS::Node::to_s/from_s", "[havannah][agentpns]") {
	AgentPNS::Node n(Move("a1"));
	auto s = n.to_s();
	AgentPNS::Node k;
	REQUIRE(k.from_s(s));
	REQUIRE(n.to_s() == k.to_s());
}
