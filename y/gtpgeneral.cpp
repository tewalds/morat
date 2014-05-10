
#include "gtp.h"
#include "lbdist.h"


GTPResponse GTP::gtp_print(vecstr args){
	Board board = game.getboard();
	for(unsigned int i = 0; i < args.size() && board.move(args[i]); i++)
		;
	return GTPResponse(true, "\n" + board.to_s(colorboard));
}

GTPResponse GTP::gtp_swap(vecstr args){
	if(args.size() == 0)
		return GTPResponse(false, "Wrong number of arguments");

	if(args.size() >= 1)
		allow_swap = from_str<bool>(args[0]);

	string ret = "";
	if(allow_swap) ret += "Swap on";
	else           ret += "Swap off";

	return GTPResponse(true, ret);
}

GTPResponse GTP::gtp_boardsize(vecstr args){
	if(args.size() != 1)
		return GTPResponse(false, "Current board size: " + to_str(game.getsize()));

	int size = from_str<int>(args[0]);
	if(size < 3 || size > 16)
		return GTPResponse(false, "Size " + to_str(size) + " is out of range.");

	game = Game(size);
	set_board();

	time_control.new_game();

	return GTPResponse(true);
}

GTPResponse GTP::gtp_clearboard(vecstr args){
	game.clear();
	set_board();

	time_control.new_game();

	return GTPResponse(true);
}

GTPResponse GTP::gtp_undo(vecstr args){
	int num = 1;
	if(args.size() >= 1)
		num = from_str<int>(args[0]);

	while(num--){
		game.undo();
	}
	set_board(false);
	if(verbose >= 2)
		logerr(game.getboard().to_s(colorboard) + "\n");
	return GTPResponse(true);
}

GTPResponse GTP::gtp_patterns(vecstr args){
	bool symmetric = true;
	bool invert = true;
	string ret;
	Board board = game.getboard();
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
	Board board = game.getboard();
	for(Board::MoveIterator move = board.moveit(); !move.done(); ++move)
		ret += move->to_s() + " ";
	return GTPResponse(true, ret);
}

GTPResponse GTP::gtp_history(vecstr args){
	string ret;
	vector<Move> hist = game.get_hist();
	for(unsigned int i = 0; i < hist.size(); i++)
		ret += hist[i].to_s() + " ";
	return GTPResponse(true, ret);
}

GTPResponse GTP::play(const string & pos, int toplay){
	if(toplay != game.toplay())
		return GTPResponse(false, "It is the other player's turn!");

	if(game.getboard().won() >= 0)
		return GTPResponse(false, "The game is already over.");

	Move m(pos);

	if(!game.valid(m))
		return GTPResponse(false, "Invalid move");

	move(m);

	if(verbose >= 2)
		logerr("Placement: " + m.to_s() + ", outcome: " + game.getboard().won_str() + "\n" + game.getboard().to_s(colorboard));

	return GTPResponse(true);
}

GTPResponse GTP::gtp_playgame(vecstr args){
	GTPResponse ret(true);

	for(unsigned int i = 0; ret.success && i < args.size(); i++)
		ret = play(args[i], game.toplay());

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
	return GTPResponse(true, game.getboard().won_str());
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
	str += "Board size:  " + to_str(game.getboard().get_size()) + "\n";
	str += "Board cells: " + to_str(game.getboard().numcells()) + "\n";
	str += "Board vec:   " + to_str(game.getboard().vecsize()) + "\n";
	str += "Board mem:   " + to_str(game.getboard().memsize()) + "\n";

	return GTPResponse(true, str);
}

GTPResponse GTP::gtp_dists(vecstr args){
	Board board = game.getboard();
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

	string s = "\n";
	for(int i = 0; i < size; i++)
		s += " " + coord + to_str(i+1);
	s += "\n";

	for(int y = 0; y < size; y++){
		s += string(y, ' ');
		s += coord + char('A' + y);
		int end = board.lineend(y);
		for(int x = 0; x < end; x++){
			int p = board.get(x, y);
			s += ' ';
			if(p == 0){
				int d = (side ? dists.get(Move(x, y), side) : dists.get(Move(x, y)));
				if(d < 30)
					s += reset + to_str(d);
				else
					s += empty;
			}else if(p == 1){
				s += white;
			}else if(p == 2){
				s += black;
			}
		}
		s += '\n';
	}
	return GTPResponse(true, s);
}

GTPResponse GTP::gtp_zobrist(vecstr args){
	return GTPResponse(true, game.getboard().hashstr());
}
