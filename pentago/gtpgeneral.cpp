
#include "gtp.h"
#include "moveiterator.h"

GTPResponse GTP::gtp_mcts(vecstr args){
	delete agent;
	agent = new AgentMCTS();
	agent->set_board(game.getboard());
	return GTPResponse(true);
}

GTPResponse GTP::gtp_pns(vecstr args){
	delete agent;
	agent = new AgentPNS();
	agent->set_board(game.getboard());
	return GTPResponse(true);
}
GTPResponse GTP::gtp_ab(vecstr args){
	delete agent;
	agent = new AgentAB();
	agent->set_board(game.getboard());
	return GTPResponse(true);
}

GTPResponse GTP::gtp_state(vecstr args){
	return GTPResponse(true, game.getboard().state());
}

GTPResponse GTP::gtp_print(vecstr args){
	Board board = game.getboard();
	for(unsigned int i = 0; i < args.size() && board.move(args[i]); i++)
		;
	return GTPResponse(true, "\n" + board.to_s(colorboard));
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

GTPResponse GTP::gtp_all_legal(vecstr args){
	string ret;
	for(MoveIterator move(game.getboard()); !move.done(); ++move)
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
	return GTPResponse(true, "Pentagod");
}

GTPResponse GTP::gtp_version(vecstr args){
	return GTPResponse(true, "1.5");
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

GTPResponse GTP::gtp_hash(vecstr args){
	return GTPResponse(true, to_str(game.getboard().hash()));
}
