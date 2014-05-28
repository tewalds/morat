
#pragma once

/*
Compute a rough lower bound on the number of additional moves needed to win through this position

Done using floodfills from each edge for each player, only going in the 3 forward directions

Increase distance when crossing an opponent virtual connection?
Decrease distance when crossing your own virtual connection?
*/

//TODO: Needs to be fixed for only one direction per player

#include "board.h"
#include "move.h"

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

	int & dist(int edge, int player, int i)          { return dists[edge][player-1][i]; }
	int & dist(int edge, int player, const Move & m) { return dist(edge, player, board->xy(m)); }
	int & dist(int edge, int player, int x, int y)   { return dist(edge, player, board->xy(x, y)); }

	void init(int x, int y, int edge, int player, int dir){
		int val = board->get(x, y);
		if(val != 3 - player){
			Q.push(MoveDist(x, y, (val == 0), dir));
			dist(edge, player, x, y) = (val == 0);
		}
	}

public:

	LBDists() : board(NULL) {}
	LBDists(const Board * b) { run(b); }

	void run(const Board * b, bool crossvcs = true, int side = 0) {
		board = b;

		for(int i = 0; i < 3; i++)
			for(int j = 0; j < 2; j++)
				for(int k = 0; k < board->vecsize(); k++)
					dists[i][j][k] = maxdist; //far far away!

		int m = board->get_size();
		int m1 = m-1;

		int start, end;
		if(side){ start = end = side; }
		else    { start = 1; end = 2; }

		for(int player = start; player <= end; player++){
			for(int x = 0; x < m; x++) { init(x,    0, 0,  player, 3); } flood(0,  player, crossvcs); //edge 0
			for(int y = 0; y < m; y++) { init(0,    y, 1,  player, 1); } flood(1,  player, crossvcs); //edge 1
			for(int y = 0; y < m; y++) { init(m1-y, y, 2,  player, 5); } flood(2,  player, crossvcs); //edge 2
		}
	}

	void flood(int edge, int player, bool crossvcs){
		int otherplayer = 3 - player;

		MoveDist cur;
		while(Q.pop(cur)){
			for(int i = 5; i <= 7; i++){
				int nd = (cur.dir + i) % 6;
				MoveDist next(cur.pos + neighbours[nd], cur.dist, nd);

				if(board->onboard(next.pos)){
					int pos = board->xy(next.pos);
					int colour = board->get(pos);

					if(colour == otherplayer)
						continue;

					if(colour == 0){
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

	int get(Move pos){ return min(get(pos, 1),  get(pos, 2)); }
	int get(Move pos, int player){ return get(board->xy(pos), player); }
	int get(int pos, int player){
		int sum = 0;
		for(int i = 0; i < 3; i++)
			sum += dist(i, player, pos);
		return sum;
	}
};
