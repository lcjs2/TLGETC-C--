//Implementation of the globals.h file of global constants (which is full of extern declarations)
#include "globals.h"

const int BLACK = 0;
const int WHITE = 1;
const int FRIEND = 2;
const int ENEMY = 3;

// Directions for board-querying functions.
const int HV = 1; //Horizontal/vertical
const int DIAG = 2; //Diagonal
const int HVDIAG = 3; //Both (should be HV + DIAG for bitwise comparison)

// Alive/dead/unsettled for reading (both tactical and strategic)
const int ALIVE = 0;
const int DEAD = 1;
const int UNSETTLED = 2;

const int DEBUG_BS = 9; //Board size, for debugging only!

bool vectors_intersect(std::vector<int> vec0, std::vector<int> vec1)
{
	for(std::vector<int>::iterator ii = vec0.begin();ii!=vec0.end();ii++)
	{
		if((find(vec1.begin(), vec1.end(), *ii))!=vec1.end()) return true;
	}
	return false;

}

