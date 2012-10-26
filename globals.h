#ifndef GLOBALS_H_300712
#define GLOBALS_H_300712

#include <vector>
#include <algorithm>
#include <string>

#include "board.h"
#include <assert.h>

extern const int BLACK;
extern const int WHITE;
extern const int FRIEND;
extern const int ENEMY;

extern const int HV;
extern const int DIAG;
extern const int HVDIAG;

extern const int ALIVE;
extern const int DEAD;
extern const int UNSETTLED;

extern const int DEBUG_BS;


bool vectors_intersect(std::vector<int> vec0, std::vector<int> vec1); // THIS IS SO BAD! WHAT WERE YOU THINKING?

// Base class for move-generating engines
// Engine is going to contain a copy of the board state which gets updated with each move
// so we can store data about lumps as ponters
class Engine
{
public:
	Engine(int bsize):board_size(bsize)
	{
	}
	int board_size;
	virtual void make_move(int pos){} // Update relevant data in derived class
	virtual int get_move(void)=0; // Return best move for current player
	virtual std::string get_name(void){return "Nameless Engine";} // Return name of the engine
};









#endif