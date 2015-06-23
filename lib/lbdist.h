
#pragma once

/*
Compute a rough lower bound on the number of additional moves needed to win given this position

Done using floodfills from each edge/corner for each side, only going in the 3 forward directions

Increase distance when crossing an opponent virtual connection?
Decrease distance when crossing your own virtual connection?
*/

#include <functional>

#include "move.h"


namespace Morat {

template<class SubClass, class Board>
class LBDistsBase {
/*
This uses the Curiously recurring template pattern: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
so that this class, the super class, can call init_player and _get on the subclass. The alternative
would be to use virtual functions, which would work, but could be significantly slower.

Subclasses must implement init_player and _get which define which flood fills to run,
and how to look up the value for a given cell.
*/

	SubClass* self() {
		return static_cast<SubClass*>(this);
	}

protected:
	struct MoveDist {
		MoveValid pos;
		int dist;
		int dir;

		MoveDist() { }
		MoveDist(MoveValid p, int d, int r) : pos(p), dist(d), dir(r) { }
	};

	//a specialized priority queue
	//distances must start at 0 or 1
	//new values must be the same as current smallest, or larger but smaller than min + maxvals
	class IntPQueue {
		static const int maxvals = 4; //maximum number of distinct values that can be stored
		MoveDist vals[maxvals][Board::max_vec_size];
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

	int dists[Board::LBDist_directions][2][Board::max_vec_size]; //[edge/corner][player][cell]
	static const int maxdist = 1000;
	IntPQueue Q;
	const Board * board;

	int & dist(int edge, Side player, int i)               { return dists[edge][player.to_i() - 1][i]; }
	int & dist(int edge, Side player, const MoveValid & m) { return dist(edge, player, m.xy); }
	int & dist(int edge, Side player, const Move & m)      { return dist(edge, player, board->xy(m)); }
	int & dist(int edge, Side player, int x, int y)        { return dist(edge, player, board->xy(x, y)); }

	void init(int x, int y, int edge, Side player, int dir){
		Side val = board->get(x, y);
		if(val != ~player){
			bool empty = (val == Side::NONE);
			MoveValid move(x, y, board->xy(x, y));
			Q.push(MoveDist(move, empty, dir));
			dist(edge, player, move) = empty;
		}
	}

public:

	LBDistsBase() : board(NULL) {}
	LBDistsBase(const Board * b) { run(b); }

	void run(const Board * b, bool crossvcs = true, Side side = Side::BOTH) {
	// run the flood fills needed to generate the distances
		board = b;

		for(int i = 0; i < Board::LBDist_directions; i++)
			for(int j = 0; j < 2; j++)
				for(int k = 0; k < board->vec_size(); k++)
					dists[i][j][k] = maxdist; //far far away!

		if((side & Side::P1) == Side::P1) self()->init_player(crossvcs, Side::P1);
		if((side & Side::P2) == Side::P2) self()->init_player(crossvcs, Side::P2);
	}

	// return the distance to for a single cell, either the minimum of both sides or for the chosen side
	int get(Move      pos) { return get(MoveValid(pos, board->xy(pos))); }
	int get(MoveValid pos) { return std::min(get(pos, Side::P1), get(pos, Side::P2)); }
	int get(Move      pos, Side player) { return get(board->xy(pos), player); }
	int get(MoveValid pos, Side player) { return get(pos.xy, player); }
	int get(int pos, Side player) { return self()->_get(pos, player); }

	std::string to_s(Side side = Side::NONE) {
	// Return a string representation of the whole board, with empty cells showing their distance numbers
		if (board == NULL)
			return "LBDistsBase has no board.....\n";
		return board->to_s(true, std::bind(&LBDistsBase::get_s, this, std::placeholders::_1, side));
	}

	std::string get_s(Move pos, Side side) {
	// Return a string version of the distance for a single cell, for use by Board::to_s
		int dist = (side == Side::NONE ? get(pos) : get(pos, side));
		return (dist < 10 ? to_str(dist) : ".");
	}


protected:
	void init_player(bool crossvcs, Side player);
	int _get(int pos, Side player);

	void flood(int edge, Side player, bool crossvcs){
		Side otherplayer = ~player;

		MoveDist cur;
		while(Q.pop(cur)){
			const MoveValid* neighbors = board->neighbors(cur.pos);
			for(int i = 5; i <= 7; i++){
				int nd = (cur.dir + i) % 6;
				MoveDist next(neighbors[nd], cur.dist, nd);

				if(board->on_board(next.pos)){
					Side colour = board->get(next.pos);

					if(colour == otherplayer)
						continue;

					if(colour == Side::NONE){
						if(!crossvcs && //forms a vc
						   board->get(neighbors[(nd + 5) % 6]) == otherplayer &&
						   board->get(neighbors[(nd + 7) % 6]) == otherplayer)
							continue;

						next.dist++;
					}

					if( dist(edge, player, next.pos) > next.dist){
						dist(edge, player, next.pos) = next.dist;
						Q.push(next);
					}
				}
			}
		}
	}

	void partialsort(int * list, int max){
	//partially sort the list with selection sort
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

}; // namespace Morat
