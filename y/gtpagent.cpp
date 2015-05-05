
#include "gtp.h"


namespace Morat {
namespace Y {

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
			time_control.get_time(hist->movesremain(), agent->gamelen()));

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
			time_control.get_time(hist->movesremain(), agent->gamelen()));


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
//	if(dynamic_cast<AgentAB   *>(agent)) return gtp_ab_params(args);
	if(dynamic_cast<AgentMCTS *>(agent)) return gtp_mcts_params(args);
	if(dynamic_cast<AgentPNS  *>(agent)) return gtp_pns_params(args);

	return GTPResponse(false, "Unknown Agent type");
}
/*
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
*/
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
			"Final move selection:\n" +
			"  -E --msexplore   Lower bound constant in final move selection      [" + to_str(mcts->msexplore) + "]\n" +
			"  -F --msrave      Rave factor, 0 for pure exp, -1 # sims, -2 # wins [" + to_str(mcts->msrave) + "]\n" +
			"Tree traversal:\n" +
			"  -e --explore     Exploration rate for UCT                          [" + to_str(mcts->explore) + "]\n" +
			"  -A --parexplore  Multiply the explore rate by parents experience   [" + to_str(mcts->parentexplore) + "]\n" +
			"  -f --ravefactor  The rave factor: alpha = rf/(rf + visits)         [" + to_str(mcts->ravefactor) + "]\n" +
			"  -d --decrrave    Decrease the rave factor over time: rf += d*empty [" + to_str(mcts->decrrave) + "]\n" +
			"  -a --knowledge   Use knowledge: 0.01*know/sqrt(visits+1)           [" + to_str(mcts->knowledge) + "]\n" +
			"  -r --userave     Use rave with this probability [0-1]              [" + to_str(mcts->userave) + "]\n" +
			"  -X --useexplore  Use exploration with this probability [0-1]       [" + to_str(mcts->useexplore) + "]\n" +
			"  -u --fpurgency   Value to assign to an unplayed move               [" + to_str(mcts->fpurgency) + "]\n" +
			"  -O --rollouts    Number of rollouts to run per simulation          [" + to_str(mcts->rollouts) + "]\n" +
			"  -I --dynwiden    Dynamic widening, consider log_wid(exp) children  [" + to_str(mcts->dynwiden) + "]\n" +
			"Tree building:\n" +
			"  -s --shortrave   Only use moves from short rollouts for rave       [" + to_str(mcts->shortrave) + "]\n" +
			"  -k --keeptree    Keep the tree from the previous move              [" + to_str(mcts->keeptree) + "]\n" +
			"  -m --minimax     Backup the minimax proof in the UCT tree          [" + to_str(mcts->minimax) + "]\n" +
			"  -x --visitexpand Number of visits before expanding a node          [" + to_str(mcts->visitexpand) + "]\n" +
			"  -P --symmetry    Prune symmetric moves, good for proof, not play   [" + to_str(mcts->prunesymmetry) + "]\n" +
			"     --gcsolved    Garbage collect solved nodes with fewer sims than [" + to_str(mcts->gcsolved) + "]\n" +
			"  -L --longestloss For known losses take longest over hardest solve  [" + to_str(mcts->longestloss)+"]\n"+

			"Node initialization knowledge, Give a bonus:\n" +
			"  -l --localreply  based on the distance to the previous move        [" + to_str(mcts->localreply) + "]\n" +
			"  -y --locality    to stones near other stones of the same color     [" + to_str(mcts->locality) + "]\n" +
			"  -c --connect     to stones connected to edges/corners              [" + to_str(mcts->connect) + "]\n" +
			"  -S --size        based on the size of the group                    [" + to_str(mcts->size) + "]\n" +
			"  -b --bridge      to maintaining a 2-bridge after the op probes     [" + to_str(mcts->bridge) + "]\n" +
			"  -D --distance    to low minimum distance to win (<0 avoid VCs)     [" + to_str(mcts->dists) + "]\n" +
			"Rollout policy:\n" +
			"  -h --weightrand  Weight the moves according to computed gammas     [" + to_str(mcts->weightedrandom) + "]\n" +
			"  -p --pattern     Maintain the virtual connection pattern           [" + to_str(mcts->rolloutpattern) + "]\n" +
			"  -g --goodreply   Reuse the last good reply (1), remove losses (2)  [" + to_str(mcts->lastgoodreply) + "]\n" +
			"  -w --instantwin  Look for instant wins to this depth               [" + to_str(mcts->instantwin) + "]\n"
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
		}else if((arg == "-E" || arg == "--msexplore") && i+1 < args.size()){
			mcts->msexplore = from_str<float>(args[++i]);
		}else if((arg == "-F" || arg == "--msrave") && i+1 < args.size()){
			mcts->msrave = from_str<float>(args[++i]);
		}else if((arg == "-e" || arg == "--explore") && i+1 < args.size()){
			mcts->explore = from_str<float>(args[++i]);
		}else if((arg == "-A" || arg == "--parexplore") && i+1 < args.size()){
			mcts->parentexplore = from_str<bool>(args[++i]);
		}else if((arg == "-f" || arg == "--ravefactor") && i+1 < args.size()){
			mcts->ravefactor = from_str<float>(args[++i]);
		}else if((arg == "-d" || arg == "--decrrave") && i+1 < args.size()){
			mcts->decrrave = from_str<float>(args[++i]);
		}else if((arg == "-a" || arg == "--knowledge") && i+1 < args.size()){
			mcts->knowledge = from_str<bool>(args[++i]);
		}else if((arg == "-s" || arg == "--shortrave") && i+1 < args.size()){
			mcts->shortrave = from_str<bool>(args[++i]);
		}else if((arg == "-k" || arg == "--keeptree") && i+1 < args.size()){
			mcts->keeptree = from_str<bool>(args[++i]);
		}else if((arg == "-m" || arg == "--minimax") && i+1 < args.size()){
			mcts->minimax = from_str<int>(args[++i]);
		}else if((arg == "-P" || arg == "--symmetry") && i+1 < args.size()){
			mcts->prunesymmetry = from_str<bool>(args[++i]);
		}else if((arg == "-L" || arg == "--longestloss") && i+1 < args.size()){
			mcts->longestloss = from_str<bool>(args[++i]); 
		}else if((               arg == "--gcsolved") && i+1 < args.size()){
			mcts->gcsolved = from_str<uint>(args[++i]);
		}else if((arg == "-r" || arg == "--userave") && i+1 < args.size()){
			mcts->userave = from_str<float>(args[++i]);
		}else if((arg == "-X" || arg == "--useexplore") && i+1 < args.size()){
			mcts->useexplore = from_str<float>(args[++i]);
		}else if((arg == "-u" || arg == "--fpurgency") && i+1 < args.size()){
			mcts->fpurgency = from_str<float>(args[++i]);
		}else if((arg == "-O" || arg == "--rollouts") && i+1 < args.size()){
			mcts->rollouts = from_str<int>(args[++i]);
			if(mcts->gclimit < mcts->rollouts*5)
				mcts->gclimit = mcts->rollouts*5;
		}else if((arg == "-I" || arg == "--dynwiden") && i+1 < args.size()){
			mcts->dynwiden = from_str<float>(args[++i]);
			mcts->logdynwiden = std::log(mcts->dynwiden);
		}else if((arg == "-x" || arg == "--visitexpand") && i+1 < args.size()){
			mcts->visitexpand = from_str<uint>(args[++i]);
		}else if((arg == "-l" || arg == "--localreply") && i+1 < args.size()){
			mcts->localreply = from_str<int>(args[++i]);
		}else if((arg == "-y" || arg == "--locality") && i+1 < args.size()){
			mcts->locality = from_str<int>(args[++i]);
		}else if((arg == "-c" || arg == "--connect") && i+1 < args.size()){
			mcts->connect = from_str<int>(args[++i]);
		}else if((arg == "-S" || arg == "--size") && i+1 < args.size()){
			mcts->size = from_str<int>(args[++i]);
		}else if((arg == "-b" || arg == "--bridge") && i+1 < args.size()){
			mcts->bridge = from_str<int>(args[++i]);
		}else if((arg == "-D" || arg == "--distance") && i+1 < args.size()){
			mcts->dists = from_str<int>(args[++i]);
		}else if((arg == "-h" || arg == "--weightrand") && i+1 < args.size()){
			mcts->weightedrandom = from_str<int>(args[++i]);
		}else if((arg == "-p" || arg == "--pattern") && i+1 < args.size()){
			mcts->rolloutpattern = from_str<bool>(args[++i]);
		}else if((arg == "-g" || arg == "--goodreply") && i+1 < args.size()){
			mcts->lastgoodreply = from_str<int>(args[++i]);
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
			"  -a --abdepth  Run an alpha-beta search of this size at each leaf       [" + to_str(pns->ab) + "]\n"
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
		}else if((arg == "-a" || arg == "--abdepth") && i+1 < args.size()){
			pns->ab = from_str<int>(args[++i]);
		}else{
			return GTPResponse(false, "Missing or unknown parameter");
		}
	}

	return GTPResponse(true, errs);
}

}; // namespace Y
}; // namespace Morat
