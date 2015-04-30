
#include "../lib/catch.hpp"

#include "agentmcts.h"
#include "gtp.h"


using namespace Morat;
using namespace Rex;

void test_playout(AgentMCTS &agent, Outcome outcome, bool fullboard){
	while(!agent.done()){
		agent.search(10, 0, 0);
	}
	while(agent.rootboard.won()==Outcome::UNKNOWN){
		agent.search(10, 0, 0);
		Move move = agent.return_move(0);
		agent.move(move);
	}
	REQUIRE(agent.rootboard.won() == outcome);
	if(fullboard){//check that board is full in the cases where loser should be able to force it
		REQUIRE(agent.rootboard.num_moves() == agent.rootboard.numcells());
	}
	logerr("Completed a test game.\n");
}

void setup_game(AgentMCTS &agent, int size, std::vector<std::string> moves){
	Board b(size);
	agent.set_board(b, true);
	agent.longestloss = true;//loser tries to draw out game
	for(auto m : moves) {
		agent.move(Move(m));
	}
}

TEST_CASE("Rex::AgentMCTS::Node::to_s/from_s", "[rex][agentmcts]") {
	AgentMCTS::Node n(Move("a1"));
	auto s = n.to_s();
	AgentMCTS::Node k;
	REQUIRE(k.from_s(s));
	REQUIRE(n.to_s() == k.to_s());
}

TEST_CASE("Rex::AgentMCTS 3x3", "[rex][agentmcts]"){
	SECTION("Unspecified Opening"){
		AgentMCTS agent;
		setup_game(agent, 3, {});//3x3 board with no specified opening
		test_playout(agent, Outcome::P2, true);//should be player2 win
	}
	SECTION("Lossing Reply"){
		AgentMCTS agent;
		setup_game(agent, 3, {"a1", "b2"});//3x3 board with acut opening and center reply
		test_playout(agent, Outcome::P1, false);//should be player1 win
	}
}

TEST_CASE("Rex::AgentMCTS 4x4", "[rex][agentmcts]"){

	SECTION("Unspecified Opening"){
		AgentMCTS agent;
		setup_game(agent, 4, {});//4x4 board with no specified opening
		test_playout(agent, Outcome::P1, true);//should be player1 win
	}

	SECTION("Obtuse Corner Opening"){
		AgentMCTS agent;
		setup_game(agent, 4, {"a4"});//4x4 board with obtuse corner opening move
		test_playout(agent, Outcome::P1, true);//should be player1 win
	}

	SECTION("Acute Corner Opening"){
		AgentMCTS agent;
		setup_game(agent, 4, {"a1"});//4x4 board with acute corner opening move
		test_playout(agent, Outcome::P1, true);//should be player1 win
	}

	SECTION("Center Opening"){
		AgentMCTS agent;
		setup_game(agent, 4, {"c2"});//4x4 board with center opening move
		test_playout(agent, Outcome::P2, false);//this opening is player2 win
	}

	SECTION("Offcenter Opening"){
		AgentMCTS agent;
		setup_game(agent, 4, {"b2"});
		test_playout(agent, Outcome::P1, true);//should be player1 win
	}

	SECTION("Adjecent to Acute Opening"){
		AgentMCTS agent;
		setup_game(agent, 4, {"b1"});
		test_playout(agent, Outcome::P1, true);//should be player1 win
	}

	SECTION("Adjecent to Obtuse Opening"){
		AgentMCTS agent;
		setup_game(agent, 4, {"a3"});
		test_playout(agent, Outcome::P1, true);//should be player1 win
	}
}
/*
TEST_CASE("Rex::AgentMCTS 5x5", "[rex][agentmcts]"){
	SECTION("Unspecified Opening/Reply"){
		AgentMCTS agent;
		setup_game(agent, 5, {});//5x5 board with no specified opening
		test_playout(agent, Outcome::P2);//should be player2 win
	}

	SECTION("Acute Opening/Reply"){
		AgentMCTS agent;
		setup_game(agent, 3, {"a1", "e5"});//3x3 board with no specified opening
		test_playout(agent, Outcome::P2);//should be player2 win
	}
}
*/


