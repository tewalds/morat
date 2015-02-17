
#pragma once

/*
Compute a rough lower bound on the number of additional moves needed to win through this position

Done using floodfills from each edge for each player, only going in the 3 forward directions

Increase distance when crossing an opponent virtual connection?
Decrease distance when crossing your own virtual connection?
*/

//TODO: Needs to be fixed for only one direction per player

#include "../lib/move.h"

#include "board.h"


namespace Morat {
namespace Hex {

class LBDists {
	struct MoveDist {
		Move pos;
		int dist;
		int dir;

		MoveDist() { }
		MoveDist(Move p, int d, int r)       : pos(p),         dist(d), dir(r) { }
		MoveDist(int x, int y, int d, int r) : pos(Move(x,y)), dist(d), dir(r) { }
	};

	//a specialized priority queue
	//distances must start at 0 or 1
	//new values must be the same as current smallest, or larger but smaller than min + maxvals
	class IntPQueue {
		static const int maxvals = 4; //maximum number of distinct values that can be stored
		MoveDist vals[maxvals][Board::max_vecsize];
		int counts[maxvals];
		int current; //which vector
		int num; //int num elements total
	public:
		IntPQueue(){
			reset();
		}
		void reset(){
			current = 0;
			num = 0;
			for(int i = 0; i < maxvals; i++)
				counts[i] = 0;
		}
		void push(const MoveDist & v){
			int i = v.dist % maxvals;
			vals[i][counts[i]++] = v;
			num++;
		}
		bool pop(MoveDist & ret){
			if(num == 0){
				current = 0;
				return false;
			}

			while(counts[current] == 0)
				current = (current+1) % maxvals;

			ret = vals[current][--counts[current]];
			num--;

			return true;
		}
	};

	int dists[3][2][Board::max_vecsize]; //[edge][player][cell]
	static const int maxdist = 1000;
	IntPQueue Q;
	const Board * board;

	int & dist(int edge, Side player, int i)          { return dists[edge][player.to_i() - 1][i]; }
	int & dist(int edge, Side player, const Move & m) { return dist(edge, player, board->xy(m)); }
	int & dist(int edge, Side player, int x, int y)   { return dist(edge, player, board->xy(x, y)); }

	void init(int x, int y, int edge, Side player, int dir){
		Side val = board->get(x, y);
		if(val != ~player){
			bool empty = (val == Side::NONE);
			Q.push(MoveDist(x, y, empty, dir));
			dist(edge, player, x, y) = empty;
		}
	}

public:

	LBDists() : board(NULL) {}
	LBDists(const Board * b) { run(b); }

	void run(const Board * b, bool crossvcs = true, Side side = Side::BOTH) {
		board = b;

		for(int i = 0; i < 3; i++)
			for(int j = 0; j < 2; j++)
				for(int k = 0; k < board->vecsize(); k++)
					dists[i][j][k] = maxdist; //far far away!

		if(side == Side::P1 || side == Side::BOTH) init_player(crossvcs, Side::P1);
		if(side == Side::P2 || side == Side::BOTH) init_player(crossvcs, Side::P2);
	}

	void init_player(bool crossvcs, Side player){
		int m = board->get_size();
		int m1 = m-1;

		for(int x = 0; x < m; x++) { init(x,    0, 0,  player, 3); } flood(0,  player, crossvcs); //edge 0
		for(int y = 0; y < m; y++) { init(0,    y, 1,  player, 1); } flood(1,  player, crossvcs); //edge 1
		for(int y = 0; y < m; y++) { init(m1-y, y, 2,  player, 5); } flood(2,  player, crossvcs); //edge 2
	}

	void flood(int edge, Side player, bool crossvcs){
		Side otherplayer = ~player;

		MoveDist cur;
		while(Q.pop(cur)){
			for(int i = 5; i <= 7; i++){
				int nd = (cur.dir + i) % 6;
				MoveDist next(cur.pos + neighbours[nd], cur.dist, nd);

				if(board->onboard(next.pos)){
					int pos = board->xy(next.pos);
					Side colour = board->get(pos);

					if(colour == otherplayer)
						continue;

					if(colour == Side::NONE){
						if(!crossvcs && //forms a vc
						   board->get(cur.pos + neighbours[(nd - 1) % 6]) == otherplayer &&
						   board->get(cur.pos + neighbours[(nd + 1) % 6]) == otherplayer)
							continue;

						next.dist++;
					}

					if( dist(edge, player, pos) > next.dist){
						dist(edge, player, pos) = next.dist;
						if(next.dist < board->get_size())
							Q.push(next);
					}
				}
			}
		}
	}

	int get(Move pos){ return std::min(get(pos, Side::P1),  get(pos, Side::P2)); }
	int get(Move pos, Side player){ return get(board->xy(pos), player); }
	int get(int pos, Side player){
		int sum = 0;
		for(int i = 0; i < 3; i++)
			sum += dist(i, player, pos);
		return sum;
	}
};

}; // namespace Hex
}; // namespace Morat
