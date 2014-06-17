
#pragma once

/*
Compute a rough lower bound on the number of additional moves needed to win given this position

Done using floodfills from each edge/corner for each side, only going in the 3 forward directions

Increase distance when crossing an opponent virtual connection?
Decrease distance when crossing your own virtual connection?
*/


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

	int dists[12][2][Board::max_vecsize]; //[edge/corner][player][cell]
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

		for(int i = 0; i < 12; i++)
			for(int j = 0; j < 2; j++)
				for(int k = 0; k < board->vecsize(); k++)
					dists[i][j][k] = maxdist; //far far away!

		if(side == Side::P1 || side == Side::BOTH) init_player(crossvcs, Side::P1);
		if(side == Side::P2 || side == Side::BOTH) init_player(crossvcs, Side::P2);
	}

	void init_player(bool crossvcs, Side player){
		int m = board->get_size()-1, e = board->get_size_d()-1;

		init(0, 0, 0, player, 3); flood(0, player, crossvcs); //corner 0
		init(m, 0, 1, player, 4); flood(1, player, crossvcs); //corner 1
		init(e, m, 2, player, 5); flood(2, player, crossvcs); //corner 2
		init(e, e, 3, player, 0); flood(3, player, crossvcs); //corner 3
		init(m, e, 4, player, 1); flood(4, player, crossvcs); //corner 4
		init(0, m, 5, player, 2); flood(5, player, crossvcs); //corner 5

		for(int x = 1; x < m; x++)   { init(x,   0, 6,  player, 3+(x==1));   } flood(6,  player, crossvcs); //edge 0
		for(int y = 1; y < m; y++)   { init(m+y, y, 7,  player, 4+(y==1));   } flood(7,  player, crossvcs); //edge 1
		for(int y = m+1; y < e; y++) { init(e,   y, 8,  player, 5+(y==m+1)); } flood(8,  player, crossvcs); //edge 2
		for(int x = m+1; x < e; x++) { init(x,   e, 9,  player, 0+(x==e-1)); } flood(9,  player, crossvcs); //edge 3
		for(int x = 1; x < m; x++)   { init(x, m+x, 10, player, 1+(x==m-1)); } flood(10, player, crossvcs); //edge 4
		for(int y = 1; y < m; y++)   { init(0,   y, 11, player, 2+(y==m-1)); } flood(11, player, crossvcs); //edge 5
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

	Outcome isdraw(){
		Outcome outcome = Outcome::DRAW;  // assume neither side can win
		for(int y = 0; y < board->get_size_d(); y++){
			for(int x = board->linestart(y); x < board->lineend(y); x++){
				Move pos(x,y);

				if(board->encirclable(pos, Side::P1) || get(pos, Side::P1) < maxdist-5)
					outcome |= Outcome::P1; // P1 can win
				if(board->encirclable(pos, Side::P2) || get(pos, Side::P2) < maxdist-5)
					outcome |= Outcome::P2; // P2 can win

				if(outcome == Outcome::DRAW2) // both can win
					return Outcome::UNKNOWN;  // so nothing is known
			}
		}
		return -outcome; // this isn't certainty, so negate
	}

	int get(Move pos){ return min(get(pos, Side::P1),  get(pos, Side::P2)); }
	int get(Move pos, Side player){ return get(board->xy(pos), player); }
	int get(int pos, Side player){
		int list[6];
		for(int i = 0; i < 6; i++)
			list[i] = dist(i, player, pos);
		partialsort(list, 2);
		int corners = list[0] + list[1] - 1;

		for(int i = 6; i < 12; i++)
			list[i-6] = dist(i, player, pos);
		partialsort(list, 3);
		int edges = list[0] + list[1] + list[2] - 2;

		return min(corners, edges);
	}

	//partially sort the list with selection sort
	void partialsort(int * list, int max){
		for(int i = 0; i < max; i++){
			int mini = i;

			for(int j = i+1; j < 6; j++)
				if(list[mini] > list[j])
					mini = j;

			if(mini != i){
				int t = list[i];
				list[i] = list[mini];
				list[mini] = t;
			}
		}
	}
};
