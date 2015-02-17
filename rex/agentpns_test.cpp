
#include "../lib/catch.hpp"

#include "agentpns.h"


using namespace Morat;
using namespace Rex;

TEST_CASE("Rex::AgentPNS::Node::to_s/from_s", "[rex][agentpns]") {
	AgentPNS::Node n(Move("a1"));
	auto s = n.to_s();
	AgentPNS::Node k;
	REQUIRE(k.from_s(s));
	REQUIRE(n.to_s() == k.to_s());
}
