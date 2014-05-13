
#include "gtp.h"
#include "lbdist.h"


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
	string ret;
	const Board & board = *hist;
	for(Board::MoveIterator move = board.moveit(); !move.done(); ++move){
		ret += move->to_s() + " ";
		unsigned int p = board.pattern(*move);
		if(symmetric)
			p = board.pattern_symmetry(p);
		if(invert && board.toplay() == 2)
			p = board.pattern_invert(p);
		ret += to_str(p);
		ret += "\n";
	}
	return GTPResponse(true, ret);
}

GTPResponse GTP::gtp_all_legal(vecstr args){
	string ret;
	for(Board::MoveIterator move = hist->moveit(); !move.done(); ++move)
		ret += move->to_s() + " ";
	return GTPResponse(true, ret);
}

GTPResponse GTP::gtp_history(vecstr args){
	string ret;
	for(auto m : hist)
		ret += m.to_s() + " ";
	return GTPResponse(true, ret);
}

GTPResponse GTP::play(const string & pos, int toplay){
	if(toplay != hist->toplay())
		return GTPResponse(false, "It is the other player's turn!");

	if(hist->won() >= 0)
		return GTPResponse(false, "The game is already over.");

	Move m(pos);

	if(!hist->valid_move(m))
		return GTPResponse(false, "Invalid move");

	move(m);

	if(verbose >= 2)
		logerr("Placement: " + m.to_s() + ", outcome: " + hist->won_str() + "\n" + hist->to_s(colorboard));

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

	char toplay = 0;
	switch(tolower(args[0][0])){
		case 'w': toplay = 1; break;
		case 'b': toplay = 2; break;
		default:
			return GTPResponse(false, "Invalid player selection");
	}

	return play(args[1], toplay);
}

GTPResponse GTP::gtp_playwhite(vecstr args){
	if(args.size() != 1)
		return GTPResponse(false, "Wrong number of arguments");

	return play(args[0], 1);
}

GTPResponse GTP::gtp_playblack(vecstr args){
	if(args.size() != 1)
		return GTPResponse(false, "Wrong number of arguments");

	return play(args[0], 2);
}

GTPResponse GTP::gtp_winner(vecstr args){
	return GTPResponse(true, hist->won_str());
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
	string str = "\n";
	str += "Board size:  " + to_str(hist->get_size()) + "\n";
	str += "Board cells: " + to_str(hist->numcells()) + "\n";
	str += "Board vec:   " + to_str(hist->vecsize()) + "\n";
	str += "Board mem:   " + to_str(hist->memsize()) + "\n";

	return GTPResponse(true, str);
}

GTPResponse GTP::gtp_dists(vecstr args){
	Board board = *hist;
	LBDists dists(&board);

	int side = 0;
	if(args.size() >= 1){
		switch(tolower(args[0][0])){
			case 'w': side = 1; break;
			case 'b': side = 2; break;
			default:
				return GTPResponse(false, "Invalid player selection");
		}
	}

	int size = board.get_size();
	int size_d = board.get_size_d();

	string s = "\n";
	s += string(size + 3, ' ');
	for(int i = 0; i < size; i++)
		s += " " + to_str(i+1);
	s += "\n";

	string white = "O", black = "@";
	if(colorboard){
		string esc = "\033", reset = esc + "[0m";
		white = esc + "[1;33m" + "@" + reset; //yellow
		black = esc + "[1;34m" + "@" + reset; //blue
	}

	for(int y = 0; y < size_d; y++){
		s += string(abs(size-1 - y) + 2, ' ');
		s += char('A' + y);
		for(int x = board.linestart(y); x < board.lineend(y); x++){
			int p = board.get(x, y);
			s += ' ';
			if(p == 0){
				int d = (side ? dists.get(Move(x, y), side) : dists.get(Move(x, y)));
				if(d < 10)
					s += to_str(d);
				else
					s += '.';
			}else if(p == 1){
				s += white;
			}else if(p == 2){
				s += black;
			}
		}
		if(y < size-1)
			s += " " + to_str(1 + size + y);
		s += '\n';
	}
	return GTPResponse(true, s);
}

GTPResponse GTP::gtp_zobrist(vecstr args){
	return GTPResponse(true, hist->hashstr());
}
