
#include "../lib/catch.hpp"

#include "agentpns.h"


using namespace Morat;
using namespace Gomoku;

TEST_CASE("Gomoku::AgentPNS::Node::to_s/from_s", "[gomoku][agentpns]") {
	AgentPNS::Node n(Move("a1"));
	auto s = n.to_s();
	AgentPNS::Node k;
	REQUIRE(k.from_s(s));
	REQUIRE(n.to_s() == k.to_s());
}
