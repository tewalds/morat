
#include "../lib/catch.hpp"

#include "agentmcts.h"

using namespace Morat;
using namespace Y;

TEST_CASE("Y::AgentMCTS::Node::to_s/from_s", "[y][agentmcts]") {
	AgentMCTS::Node n(Move("a1"));
	auto s = n.to_s();
	AgentMCTS::Node k;
	REQUIRE(k.from_s(s));
	REQUIRE(n.to_s() == k.to_s());
}
