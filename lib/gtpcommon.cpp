
#include "gtpcommon.h"

using namespace std;

GTPResponse GTPCommon::gtp_echo(vecstr args) const {
	return GTPResponse(true, implode(args, " "));
}

GTPResponse GTPCommon::gtp_time(vecstr args){
	if(args.size() == 0)
		return GTPResponse(true, string("\n") +
			"Update the time settings, eg: time -s 2.5 -m 10 -g 600 -f 1\n" +
			"Method for distributing remaining time, current: " + time_control.method_name() + " " + to_str(time_control.param) + "\n" +
			"  -p --percent  Percentage of the remaining time every move            [10.0]\n" +
			"  -e --even     Multiple of even split of the maximum  remaining moves [2.0]\n" +
			"  -s --stats    Multiple of even split of the expected remaining moves [2.0]\n" +
			"Time allocation\n" +
			"  -m --move     Time per move                                          [" + to_str(time_control.move) + "]\n" +
			"  -g --game     Time per game                                          [" + to_str(time_control.game) + "]\n" +
			"  -f --flexible Add remaining time per move to remaining time          [" + to_str(time_control.flexible) + "]\n" +
			"  -i --maxsims  Maximum number of simulations per move                 [" + to_str(time_control.max_sims) + "]\n" +
			"Current game\n" +
			"  -r --remain   Remaining time for this game                           [" + to_str(time_control.remain) + "]\n");

	for(unsigned int i = 0; i < args.size(); i++) {
		string arg = args[i];

		if(arg == "-p" || arg == "--percent"){
			time_control.method = TimeControl::PERCENT;
			time_control.param = 10;
			if(i+1 < args.size() && from_str<double>(args[i+1]) > 0) time_control.param = from_str<double>(args[++i]);
		}else if(arg == "-e" || arg == "--even"){
			time_control.method = TimeControl::EVEN;
			time_control.param = 2;
			if(i+1 < args.size() && from_str<double>(args[i+1]) > 0) time_control.param = from_str<double>(args[++i]);
		}else if(arg == "-s" || arg == "--stats"){
			time_control.method = TimeControl::STATS;
			time_control.param = 2;
			if(i+1 < args.size() && from_str<double>(args[i+1]) > 0) time_control.param = from_str<double>(args[++i]);
		}else if((arg == "-m" || arg == "--move") && i+1 < args.size()){
			time_control.move = from_str<double>(args[++i]);
		}else if((arg == "-g" || arg == "--game") && i+1 < args.size()){
			time_control.game = from_str<float>(args[++i]);
			// TODO: should this only happen if the game hasn't started yet?
			time_control.remain = time_control.game;
		}else if((arg == "-f" || arg == "--flexible") && i+1 < args.size()){
			time_control.flexible = from_str<bool>(args[++i]);
		}else if((arg == "-i" || arg == "--maxsims") && i+1 < args.size()){
			time_control.max_sims = from_str<int>(args[++i]);
		}else if((arg == "-r" || arg == "--remain") && i+1 < args.size()){
			time_control.remain = from_str<double>(args[++i]);
		}else{
			return GTPResponse(false, "Missing or unknown parameter");
		}
	}

	return GTPResponse(true);
}

string GTPCommon::won_str(int outcome) const {
	switch(outcome){
		case -3: return "none";
		case -2: return "black_or_draw";
		case -1: return "white_or_draw";
		case 0:
		case 3:  return "draw";
		case 1:  return "white";
		case 2:  return "black";
		default: return "unknown";
	}
}
