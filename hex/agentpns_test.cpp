
#include "../lib/catch.hpp"

#include "agentpns.h"


using namespace Morat;
using namespace Hex;

TEST_CASE("Hex::AgentPNS::Node::to_s/from_s", "[hex][agentpns]") {
	AgentPNS::Node n(Move("a1"));
	auto s = n.to_s();
	AgentPNS::Node k;
	REQUIRE(k.from_s(s));
	REQUIRE(n.to_s() == k.to_s());
}
