
#include "gtp.h"


namespace Morat {
namespace Pentago {

using namespace std;

GTPResponse GTP::gtp_move_stats(vecstr args){
	vector<Move> moves;
	for(auto s : args)
		moves.push_back(Move(s));
	return GTPResponse(true, agent->move_stats(moves));
}

GTPResponse GTP::gtp_solve(vecstr args){
	if(hist->won() >= 0)
		return GTPResponse(true, "resign");

	double use_time = (args.size() >= 1 ?
			from_str<double>(args[0]) :
			time_control.get_time(hist->moves_remain(), agent->gamelen()));

	if(verbose)
		logerr("time remain: " + to_str(time_control.remain, 1) + ", time: " + to_str(use_time, 3) + ", sims: " + to_str(time_control.max_sims) + "\n");

	Time start;
	agent->search(use_time, time_control.max_sims, verbose);
	time_control.use(Time() - start);


	//TODO: find the outcome, not the best move...

	Move best = agent->return_move(verbose);

	if(verbose >= 2)
		logerr(hist->to_s(colorboard) + "\n");

	return GTPResponse(true, best.to_s());
}


GTPResponse GTP::gtp_genmove(vecstr args){
	if(hist->won() >= 0)
		return GTPResponse(true, "resign");

	double use_time = (args.size() >= 2 ?
			from_str<double>(args[1]) :
			time_control.get_time(hist->moves_remain(), agent->gamelen()));


	if(verbose)
		logerr("time:        remain: " + to_str(time_control.remain, 1) + ", use: " + to_str(use_time, 3) + ", sims: " + to_str(time_control.max_sims) + "\n");

	Time start;
	agent->search(use_time, time_control.max_sims, verbose);
	time_control.use(Time() - start);


	Move best = agent->return_move(verbose);

	move(best);

	if(verbose >= 2)
		logerr(hist->to_s(colorboard) + "\n");

	return GTPResponse(true, best.to_s());
}


GTPResponse GTP::gtp_pv(vecstr args){
	string pvstr = "";
	vector<Move> pv = agent->get_pv();
	for(unsigned int i = 0; i < pv.size(); i++)
		pvstr += pv[i].to_s() + " ";
	return GTPResponse(true, pvstr);
}

GTPResponse GTP::gtp_params(vecstr args){
	if(dynamic_cast<AgentAB   *>(agent)) return gtp_ab_params(args);
	if(dynamic_cast<AgentMCTS *>(agent)) return gtp_mcts_params(args);
	if(dynamic_cast<AgentPNS  *>(agent)) return gtp_pns_params(args);

	return GTPResponse(false, "Unknown Agent type");
}

GTPResponse GTP::gtp_ab_params(vecstr args){
	AgentAB * ab =  dynamic_cast<AgentAB *>(agent);

	if(args.size() == 0)
		return GTPResponse(true, string("\n") +
			"Set player parameters, eg: params -r 4\n" +
			"  -M --maxmem      Max memory in Mb to use for the tree              [" + to_str(ab->memlimit/(1024*1024)) + "]\n" +
			"  -r --randomness  How many bits of randomness to add to the eval    [" + to_str(ab->randomness) + "]\n"
			);

	string errs;
	for(unsigned int i = 0; i < args.size(); i++) {
		string arg = args[i];

		if((arg == "-M" || arg == "--maxmem") && i+1 < args.size()){
			ab->set_memlimit(from_str<uint64_t>(args[++i])*1024*1024);
		}else if((arg == "-r" || arg == "--randomness") && i+1 < args.size()){
			ab->randomness = from_str<int>(args[++i]);
		}else{
			return GTPResponse(false, "Missing or unknown parameter");
		}
	}
	return GTPResponse(true, errs);
}

GTPResponse GTP::gtp_mcts_params(vecstr args){
	AgentMCTS * mcts =  dynamic_cast<AgentMCTS *>(agent);

	if(args.size() == 0)
		return GTPResponse(true, string("\n") +
			"Set player parameters, eg: player_params -e 1 -f 0 -t 2 -o 1 -p 0\n" +
			"Processing:\n" +
#ifndef SINGLE_THREAD
			"  -t --threads     Number of MCTS threads                            [" + to_str(mcts->numthreads) + "]\n" +
#endif
			"  -o --ponder      Continue to ponder during the opponents time      [" + to_str(mcts->ponder) + "]\n" +
			"  -M --maxmem      Max memory in Mb to use for the tree              [" + to_str(mcts->maxmem/(1024*1024)) + "]\n" +
			"     --profile     Output the time used by each phase of MCTS        [" + to_str(mcts->profile) + "]\n" +
			"Tree traversal:\n" +
			"  -e --explore     Exploration rate for UCT                          [" + to_str(mcts->explore) + "]\n" +
			"  -A --parexplore  Multiply the explore rate by parents experience   [" + to_str(mcts->parentexplore) + "]\n" +
			"  -a --knowledge   Use knowledge: 0.01*know/sqrt(visits+1)           [" + to_str(mcts->knowledge) + "]\n" +
			"  -X --useexplore  Use exploration with this probability [0-1]       [" + to_str(mcts->useexplore) + "]\n" +
			"  -u --fpurgency   Value to assign to an unplayed move               [" + to_str(mcts->fpurgency) + "]\n" +
			"  -O --rollouts    Number of rollouts to run per simulation          [" + to_str(mcts->rollouts) + "]\n" +
			"Tree building:\n" +
			"  -k --keeptree    Keep the tree from the previous move              [" + to_str(mcts->keeptree) + "]\n" +
			"  -m --minimax     Backup the minimax proof in the UCT tree          [" + to_str(mcts->minimax) + "]\n" +
			"  -x --visitexpand Number of visits before expanding a node          [" + to_str(mcts->visitexpand) + "]\n" +
			"  -P --symmetry    Prune symmetric moves, good for proof, not play   [" + to_str(mcts->prunesymmetry) + "]\n" +
			"Rollout policy:\n" +
			"  -w --instantwin  Look for instant wins (1) and forced replies (2)  [" + to_str(mcts->instantwin) + "]\n"
			);

	string errs;
	for(unsigned int i = 0; i < args.size(); i++) {
		string arg = args[i];

		if((arg == "-t" || arg == "--threads") && i+1 < args.size()){
			mcts->pool.pause();
			mcts->numthreads = from_str<int>(args[++i]);
			mcts->pool.set_num_threads(mcts->numthreads);
			if(mcts->ponder)
				mcts->pool.resume();
		}else if((arg == "-o" || arg == "--ponder") && i+1 < args.size()){
			mcts->set_ponder(from_str<bool>(args[++i]));
		}else if((arg == "--profile") && i+1 < args.size()){
			mcts->profile = from_str<bool>(args[++i]);
		}else if((arg == "-M" || arg == "--maxmem") && i+1 < args.size()){
			mcts->maxmem = from_str<uint64_t>(args[++i])*1024*1024;
		}else if((arg == "-e" || arg == "--explore") && i+1 < args.size()){
			mcts->explore = from_str<float>(args[++i]);
		}else if((arg == "-A" || arg == "--parexplore") && i+1 < args.size()){
			mcts->parentexplore = from_str<bool>(args[++i]);
		}else if((arg == "-a" || arg == "--knowledge") && i+1 < args.size()){
			mcts->knowledge = from_str<bool>(args[++i]);
		}else if((arg == "-k" || arg == "--keeptree") && i+1 < args.size()){
			mcts->keeptree = from_str<bool>(args[++i]);
		}else if((arg == "-m" || arg == "--minimax") && i+1 < args.size()){
			mcts->minimax = from_str<int>(args[++i]);
		}else if((arg == "-P" || arg == "--symmetry") && i+1 < args.size()){
			mcts->prunesymmetry = from_str<bool>(args[++i]);
		}else if((arg == "-X" || arg == "--useexplore") && i+1 < args.size()){
			mcts->useexplore = from_str<float>(args[++i]);
		}else if((arg == "-u" || arg == "--fpurgency") && i+1 < args.size()){
			mcts->fpurgency = from_str<float>(args[++i]);
		}else if((arg == "-O" || arg == "--rollouts") && i+1 < args.size()){
			mcts->rollouts = from_str<int>(args[++i]);
		}else if((arg == "-x" || arg == "--visitexpand") && i+1 < args.size()){
			mcts->visitexpand = from_str<uint>(args[++i]);
		}else if((arg == "-w" || arg == "--instantwin") && i+1 < args.size()){
			mcts->instantwin = from_str<int>(args[++i]);
		}else{
			return GTPResponse(false, "Missing or unknown parameter");
		}
	}
	return GTPResponse(true, errs);
}

GTPResponse GTP::gtp_pns_params(vecstr args){
	AgentPNS * pns =  dynamic_cast<AgentPNS *>(agent);

	if(args.size() == 0)
		return GTPResponse(true, string("\n") +
			"Update the pns solver settings, eg: pns_params -m 100 -s 0 -d 1 -e 0.25 -a 2 -l 0\n"
			"  -m --memory   Memory limit in Mb                                       [" + to_str(pns->memlimit/(1024*1024)) + "]\n"
			"  -t --threads  How many threads to run                                  [" + to_str(pns->numthreads) + "]\n"
			"  -s --ties     Which side to assign ties to, 0 = handle, 1 = p1, 2 = p2 [" + to_str(pns->ties.to_i()) + "]\n"
			"  -d --df       Use depth-first thresholds                               [" + to_str(pns->df) + "]\n"
			"  -e --epsilon  How big should the threshold be                          [" + to_str(pns->epsilon) + "]\n"
//			"  -a --abdepth  Run an alpha-beta search of this size at each leaf       [" + to_str(pns->ab) + "]\n"
			);

	string errs;
	for(unsigned int i = 0; i < args.size(); i++) {
		string arg = args[i];

		if((arg == "-t" || arg == "--threads") && i+1 < args.size()){
			pns->numthreads = from_str<int>(args[++i]);
			pns->pool.set_num_threads(pns->numthreads);
		}else if((arg == "-m" || arg == "--memory") && i+1 < args.size()){
			uint64_t mem = from_str<uint64_t>(args[++i]);
			if(mem < 1) return GTPResponse(false, "Memory can't be less than 1mb");
			pns->set_memlimit(mem*1024*1024);
		}else if((arg == "-s" || arg == "--ties") && i+1 < args.size()){
			pns->ties = Side(from_str<int8_t>(args[++i]));
			pns->clear_mem();
		}else if((arg == "-d" || arg == "--df") && i+1 < args.size()){
			pns->df = from_str<bool>(args[++i]);
		}else if((arg == "-e" || arg == "--epsilon") && i+1 < args.size()){
			pns->epsilon = from_str<float>(args[++i]);
//		}else if((arg == "-a" || arg == "--abdepth") && i+1 < args.size()){
//			pns->ab = from_str<int>(args[++i]);
		}else{
			return GTPResponse(false, "Missing or unknown parameter");
		}
	}

	return GTPResponse(true, errs);
}

}; // namespace Pentago
}; // namespace Morat
