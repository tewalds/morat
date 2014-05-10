
#pragma once

#include "../lib/gtpbase.h"
#include "../lib/string.h"

#include "agent.h"
#include "agentab.h"
#include "agentmcts.h"
#include "agentpns.h"
#include "board.h"
#include "game.h"
#include "move.h"

struct TimeControl {
	enum Method { PERCENT, EVEN, STATS };
	Method method; //method to use to distribute the remaining time
	double param;  //param for the method, such as the percentage used or multiple of even
	double game;
	double move;
	bool   flexible; //whether time_per_move can be saved for future moves
	int    max_sims;

	TimeControl(){
		method   = STATS;
		param    = 2;
		game     = 0;
		move     = 5;
		flexible = true;
		max_sims = 0;
	}

	string method_name(){
		switch(method){
			case PERCENT: return "percent";
			case EVEN:    return "even";
			case STATS:   return "stats";
			default:      return "WTF? unknown time control method";
		}
	}
};

class GTP : public GTPBase {
	Game game;

public:
	int verbose;
	bool colorboard;

	TimeControl time;
	double      time_remain; //time remaining for this game
	int mem_allowed;

	Agent * agent;

	GTP(FILE * i = stdin, FILE * o = stdout) : GTPBase(i, o) {
		verbose = 1;
		colorboard = true;

		time_remain = time.game;
		mem_allowed = 1000;

		agent = new AgentMCTS();

		set_board();

		newcallback("name",            bind(&GTP::gtp_name,          this, _1), "Name of the program");
		newcallback("version",         bind(&GTP::gtp_version,       this, _1), "Version of the program");
		newcallback("verbose",         bind(&GTP::gtp_verbose,       this, _1), "Set verbosity, 0 for quiet, 1 for normal, 2+ for more output");
		newcallback("echo",            bind(&GTP::gtp_echo,          this, _1), "Return the arguments as the response");
		newcallback("colorboard",      bind(&GTP::gtp_colorboard,    this, _1), "Turn on or off the colored board");
		newcallback("showboard",       bind(&GTP::gtp_print,         this, _1), "Show the board");
		newcallback("print",           bind(&GTP::gtp_print,         this, _1), "Alias for showboard");
		newcallback("state",           bind(&GTP::gtp_state,         this, _1), "Output the board state in a simpler form than print, or set the board state");
		newcallback("hash",            bind(&GTP::gtp_hash,          this, _1), "Output the hash for the current position");
		newcallback("clear_board",     bind(&GTP::gtp_clearboard,    this, _1), "Clear the board, but keep the size");
		newcallback("clear",           bind(&GTP::gtp_clearboard,    this, _1), "Alias for clear_board");
		newcallback("boardsize",       bind(&GTP::gtp_clearboard,    this, _1), "Alias for clear_board, board is fixed size");
		newcallback("size",            bind(&GTP::gtp_clearboard,    this, _1), "Alias for board_size");
		newcallback("play",            bind(&GTP::gtp_play,          this, _1), "Place a stone: play <color> <location>");
		newcallback("white",           bind(&GTP::gtp_playwhite,     this, _1), "Place a white stone: white <location>");
		newcallback("black",           bind(&GTP::gtp_playblack,     this, _1), "Place a black stone: black <location>");
		newcallback("undo",            bind(&GTP::gtp_undo,          this, _1), "Undo one or more moves: undo [amount to undo]");
		newcallback("time",            bind(&GTP::gtp_time,          this, _1), "Set the time limits and the algorithm for per game time");
		newcallback("genmove",         bind(&GTP::gtp_genmove,       this, _1), "Generate a move: genmove [color] [time]");
		newcallback("solve",           bind(&GTP::gtp_solve,         this, _1), "Try to solve this position");

		newcallback("mcts",            bind(&GTP::gtp_mcts,          this, _1), "Switch to use the Monte Carlo Tree Search agent to play/solve");
		newcallback("pns",             bind(&GTP::gtp_pns,           this, _1), "Switch to use the Proof Number Search agent to play/solve");
		newcallback("ab",              bind(&GTP::gtp_ab,            this, _1), "Switch to use the Alpha/Beta agent to play/solve");

		newcallback("all_legal",       bind(&GTP::gtp_all_legal,     this, _1), "List all legal moves");
		newcallback("history",         bind(&GTP::gtp_history,       this, _1), "List of played moves");
		newcallback("playgame",        bind(&GTP::gtp_playgame,      this, _1), "Play a list of moves");
		newcallback("winner",          bind(&GTP::gtp_winner,        this, _1), "Check the winner of the game");

		newcallback("pv",              bind(&GTP::gtp_pv,            this, _1), "Output the principle variation for the player tree as it stands now");
		newcallback("move_stats",      bind(&GTP::gtp_move_stats,    this, _1), "Output the move stats for the player tree as it stands now");

		newcallback("params",          bind(&GTP::gtp_params,        this, _1), "Set the options for the player, no args gives options");
	}

	void set_board(bool clear = true){
		agent->set_board(game.getboard());
	}

	void move(const Move & m){
		game.move(m);
		agent->move(m);
	}

	GTPResponse gtp_echo(vecstr args);
	GTPResponse gtp_state(vecstr args);
	GTPResponse gtp_print(vecstr args);
	GTPResponse gtp_hash(vecstr args);
	string won_str(int outcome) const;
	GTPResponse gtp_swap(vecstr args);
	GTPResponse gtp_boardsize(vecstr args);
	GTPResponse gtp_clearboard(vecstr args);
	GTPResponse gtp_undo(vecstr args);
	GTPResponse gtp_all_legal(vecstr args);
	GTPResponse gtp_history(vecstr args);
	GTPResponse play(const string & pos, int toplay);
	GTPResponse gtp_playgame(vecstr args);
	GTPResponse gtp_play(vecstr args);
	GTPResponse gtp_playwhite(vecstr args);
	GTPResponse gtp_playblack(vecstr args);
	GTPResponse gtp_winner(vecstr args);
	GTPResponse gtp_name(vecstr args);
	GTPResponse gtp_version(vecstr args);
	GTPResponse gtp_verbose(vecstr args);
	GTPResponse gtp_colorboard(vecstr args);
	GTPResponse gtp_mcts(vecstr args);
	GTPResponse gtp_pns(vecstr args);
	GTPResponse gtp_ab(vecstr args);

	GTPResponse gtp_time(vecstr args);
	double get_time();
	void time_used(double used);
	GTPResponse gtp_move_stats(vecstr args);
	GTPResponse gtp_pv(vecstr args);
	GTPResponse gtp_genmove(vecstr args);
	GTPResponse gtp_solve(vecstr args);
	GTPResponse gtp_params(vecstr args);
	GTPResponse gtp_ab_params(vecstr args);
	GTPResponse gtp_mcts_params(vecstr args);
	GTPResponse gtp_pns_params(vecstr args);
};
