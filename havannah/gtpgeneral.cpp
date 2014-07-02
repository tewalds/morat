
#include <fstream>

#include "../lib/sgf.h"

#include "gtp.h"
#include "lbdist.h"


namespace Morat {
namespace Havannah {

GTPResponse GTP::gtp_mcts(vecstr args){
	delete agent;
	agent = new AgentMCTS();
	agent->set_board(*hist);
	return GTPResponse(true);
}

GTPResponse GTP::gtp_pns(vecstr args){
	delete agent;
	agent = new AgentPNS();
	agent->set_board(*hist);
	return GTPResponse(true);
}
/*
GTPResponse GTP::gtp_ab(vecstr args){
	delete agent;
	agent = new AgentAB();
	agent->set_board(*hist);
	return GTPResponse(true);
}
*/
GTPResponse GTP::gtp_print(vecstr args){
	Board board = *hist;
	for(auto arg : args)
		if (!board.move(arg))
			break;
	return GTPResponse(true, "\n" + board.to_s(colorboard));
}

GTPResponse GTP::gtp_boardsize(vecstr args){
	if(args.size() != 1)
		return GTPResponse(false, "Current board size: " + to_str(hist->get_size()));

	int size = from_str<int>(args[0]);
	if(size < Board::min_size || size > Board::max_size)
		return GTPResponse(false, "Size " + to_str(size) + " is out of range.");

	hist = History(size);
	set_board();
	time_control.new_game();

	return GTPResponse(true);
}

GTPResponse GTP::gtp_clearboard(vecstr args){
	hist.clear();
	set_board();
	time_control.new_game();

	return GTPResponse(true);
}

GTPResponse GTP::gtp_undo(vecstr args){
	int num = (args.size() >= 1 ? from_str<int>(args[0]) : 1);

	while(num--){
		hist.undo();
	}
	set_board(false);
	if(verbose >= 2)
		logerr(hist->to_s(colorboard) + "\n");
	return GTPResponse(true);
}

GTPResponse GTP::gtp_patterns(vecstr args){
	bool symmetric = true;
	bool invert = true;
	std::string ret;
	const Board & board = *hist;
	for(Board::MoveIterator move = board.moveit(); !move.done(); ++move){
		ret += move->to_s() + " ";
		unsigned int p = board.pattern(*move);
		if(symmetric)
			p = board.pattern_symmetry(p);
		if(invert && board.toplay() == Side::P2)
			p = board.pattern_invert(p);
		ret += to_str(p);
		ret += "\n";
	}
	return GTPResponse(true, ret);
}

GTPResponse GTP::gtp_all_legal(vecstr args){
	std::string ret;
	for(Board::MoveIterator move = hist->moveit(); !move.done(); ++move)
		ret += move->to_s() + " ";
	return GTPResponse(true, ret);
}

GTPResponse GTP::gtp_history(vecstr args){
	std::string ret;
	for(auto m : hist)
		ret += m.to_s() + " ";
	return GTPResponse(true, ret);
}

GTPResponse GTP::play(const std::string & pos, Side toplay){
	if(toplay != hist->toplay())
		return GTPResponse(false, "It is the other player's turn!");

	if(hist->won() >= Outcome::DRAW)
		return GTPResponse(false, "The game is already over.");

	Move m(pos);

	if(!hist->valid_move(m))
		return GTPResponse(false, "Invalid move");

	move(m);

	if(verbose >= 2)
		logerr("Placement: " + m.to_s() + ", outcome: " + hist->won().to_s() + "\n" + hist->to_s(colorboard));

	return GTPResponse(true);
}

GTPResponse GTP::gtp_playgame(vecstr args){
	GTPResponse ret(true);

	for(unsigned int i = 0; ret.success && i < args.size(); i++)
		ret = play(args[i], hist->toplay());

	return ret;
}

GTPResponse GTP::gtp_play(vecstr args){
	if(args.size() != 2)
		return GTPResponse(false, "Wrong number of arguments");

	switch(tolower(args[0][0])){
		case 'w': return play(args[1], Side::P1);
		case 'b': return play(args[1], Side::P2);
		default:  return GTPResponse(false, "Invalid player selection");
	}
}

GTPResponse GTP::gtp_playwhite(vecstr args){
	if(args.size() != 1)
		return GTPResponse(false, "Wrong number of arguments");

	return play(args[0], Side::P1);
}

GTPResponse GTP::gtp_playblack(vecstr args){
	if(args.size() != 1)
		return GTPResponse(false, "Wrong number of arguments");

	return play(args[0], Side::P2);
}

GTPResponse GTP::gtp_winner(vecstr args){
	return GTPResponse(true, hist->won().to_s());
}

GTPResponse GTP::gtp_name(vecstr args){
	return GTPResponse(true, "Castro");
}

GTPResponse GTP::gtp_version(vecstr args){
	return GTPResponse(true, "0.1");
}

GTPResponse GTP::gtp_verbose(vecstr args){
	if(args.size() >= 1)
		verbose = from_str<int>(args[0]);
	else
		verbose = !verbose;
	return GTPResponse(true, "Verbose " + to_str(verbose));
}

GTPResponse GTP::gtp_colorboard(vecstr args){
	if(args.size() >= 1)
		colorboard = from_str<int>(args[0]);
	else
		colorboard = !colorboard;
	return GTPResponse(true, "Color " + to_str(colorboard));
}

GTPResponse GTP::gtp_extended(vecstr args){
	if(args.size() >= 1)
		genmoveextended = from_str<bool>(args[0]);
	else
		genmoveextended = !genmoveextended;
	return GTPResponse(true, "extended " + to_str(genmoveextended));
}

GTPResponse GTP::gtp_debug(vecstr args){
	std::string str = "\n";
	str += "Board size:  " + to_str(hist->get_size()) + "\n";
	str += "Board cells: " + to_str(hist->numcells()) + "\n";
	str += "Board vec:   " + to_str(hist->vecsize()) + "\n";
	str += "Board mem:   " + to_str(hist->memsize()) + "\n";

	return GTPResponse(true, str);
}

GTPResponse GTP::gtp_dists(vecstr args){
	using std::string;
	Board board = *hist;
	LBDists dists(&board);

	Side side = Side::NONE;
	if(args.size() >= 1){
		switch(tolower(args[0][0])){
			case 'w': side = Side::P1; break;
			case 'b': side = Side::P2; break;
			default:
				return GTPResponse(false, "Invalid player selection");
		}
	}

	string white = "O",
	       black = "@",
	       empty = ".",
	       coord = "",
	       reset = "";
	if(colorboard){
		string esc = "\033";
		reset = esc + "[0m";
		coord = esc + "[1;37m";
		empty = reset + ".";
		white = esc + "[1;33m" + "@"; //yellow
		black = esc + "[1;34m" + "@"; //blue
	}


	int size = board.get_size();
	int size_d = board.get_size_d();

	string s = "\n";
	s += string(size + 3, ' ');
	for(int i = 0; i < size; i++)
		s += " " + coord + to_str(i+1);
	s += "\n";

	for(int y = 0; y < size_d; y++){
		s += string(abs(size-1 - y) + 2, ' ');
		s += coord + char('A' + y);
		for(int x = board.linestart(y); x < board.lineend(y); x++){
			Side p = board.get(x, y);
			s += ' ';
			if(p == Side::NONE){
				int d = (side == Side::NONE ? dists.get(Move(x, y)) : dists.get(Move(x, y), side));
				if(d < 10)
					s += reset + to_str(d);
				else
					s += empty;
			}else if(p == Side::P1){
				s += white;
			}else if(p == Side::P2){
				s += black;
			}
		}
		if(y < size-1)
			s += " " + coord + to_str(1 + size + y);
		s += '\n';
	}
	return GTPResponse(true, s);
}

GTPResponse GTP::gtp_zobrist(vecstr args){
	return GTPResponse(true, hist->hashstr());
}

GTPResponse GTP::gtp_save_sgf(vecstr args){
	int limit = -1;
	if(args.size() == 0)
		return GTPResponse(true, "save_sgf <filename> [work limit]");

	std::ifstream infile(args[0].c_str());

	if(infile) {
		infile.close();
		return GTPResponse(false, "File " + args[0] + " already exists");
	}

	std::ofstream outfile(args[0].c_str());

	if(!outfile)
		return GTPResponse(false, "Opening file " + args[0] + " for writing failed");

	if(args.size() > 1)
		limit = from_str<unsigned int>(args[1]);

	SGFPrinter<Move> sgf(outfile);
	sgf.game("havannah");
	sgf.program(gtp_name(vecstr()).response, gtp_version(vecstr()).response);
	sgf.size(hist->get_size());

	sgf.end_root();

	Side s = Side::P1;
	for(auto m : hist){
		sgf.move(s, m);
		s = ~s;
	}

	agent->gen_sgf(sgf, limit);

	sgf.end();
	outfile.close();
	return true;
}


GTPResponse GTP::gtp_load_sgf(vecstr args){
	if(args.size() == 0)
		return GTPResponse(true, "load_sgf <filename>");

	std::ifstream infile(args[0].c_str());

	if(!infile) {
		return GTPResponse(false, "Error opening file " + args[0] + " for reading");
	}

	SGFParser<Move> sgf(infile);
	if(sgf.game() != "havannah"){
		infile.close();
		return GTPResponse(false, "File is for the wrong game: " + sgf.game());
	}

	int size = sgf.size();
	if(size != hist->get_size()){
		if(hist.len() == 0){
			hist = History(size);
			set_board();
			time_control.new_game();
		}else{
			infile.close();
			return GTPResponse(false, "File has the wrong boardsize to match the existing game");
		}
	}

	Side s = Side::P1;

	while(sgf.next_node()){
		Move m = sgf.move();
		move(m); // push the game forward
		s = ~s;
	}

	if(sgf.has_children())
		agent->load_sgf(sgf);

	assert(sgf.done_child());
	infile.close();
	return true;
}

}; // namespace Havannah
}; // namespace Morat
