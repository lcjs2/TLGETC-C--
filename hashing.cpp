#include <vector>
#include <unordered_map>
#include "hashing.h"
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <ctime>
#include "board.h"
#include "globals.h"

using namespace std;

// Possible function calls to be stored:
// can_capture
// can_escape
// is_ladderable (worth it? Probably)

// Invincible stones not treated differently

// Hash table has 64-bit entry: 
// 32 bits for board position
// 9 bits for the target of the function call
// 1 bit for can_capture / can_escape
// = 42 bits total

// A can_capture=true or can_escape=false is always correct
// If can_capture=false or can_escape=true, we might have just exceeded depth limit

// The hash table stores a hash value representing a query, and an integer representing a result.
// If the result is "captured", then a zero is stored.
// (No depth is necessary in this case, because captured is a positive result)
// If the result is "not captured", the search depth is stored (because this result
// might have been caused by running out of depth, and so might be wrong).


ZobristHash::ZobristHash(int board_size)
{
	queries=0;
	positive_queries=0;
	srand(0);
	//srand((unsigned)time(0));
	// Fill list with 32-bit values, for b and w stones
	for(int i=0; i<2*board_size*board_size; i++)
	{
		white_values.push_back(rand() ^ (rand()<<5) ^ (rand()<<17));
		black_values.push_back(rand() ^ (rand()<<5) ^ (rand()<<17));
	}
}

void ZobristHash::print_zobrist_values(void)
{
	for(vector<int>::iterator ii=white_values.begin();ii!=white_values.end();ii++)
	{cout<<"\n"<<(*ii);}
	for(vector<int>::iterator ii=black_values.begin();ii!=black_values.end();ii++)
	{cout<<"\n"<<(*ii);}
}

// Query function for hash takes a board state b, a target position, and a search depth.
// Set want_capturable to true to query can_capture; set to false to query can_escape.
// Returns ALIVE or DEAD if it can. Returns UNSETTLED if no result or result has insufficient depth.
// The target position is replaced with the stone in the same lump whose board position is lowest
// in case the same question is asked about a different stone in the lump.
int ZobristHash::query_hash(BoardState& b, bool want_capturable, int target, int depth)
{
	//return UNSETTLED;
	//target = *(min_element(b.board[target]->stones.begin(), b.board[target]->stones.end()));
	queries++;
	_int64 query_hash = (_int64)b.hash_value;
	query_hash = query_hash ^ (((_int64)target)<<33); // Target takes at most 9 bits
	if(want_capturable)
	{
		query_hash = query_hash ^ ((_int64)1)<<(33+9);
	}
	unordered_map<_int64, int>::iterator ii=hash_table.find(query_hash);

	if(ii!=hash_table.end())
	{
		if((*ii).second==0) 
		{			
			positive_queries++; 
			return DEAD;
		}
		else if((*ii).second >= depth)
		{
			positive_queries++; 
			return ALIVE;
		}
	}
	return UNSETTLED;
}

// Add an entry to hash table
void ZobristHash::insert_hash(BoardState& b, bool want_capturable, int target, int depth, bool result)
{
	//target = *(min_element(b.board[target]->stones.begin(), b.board[target]->stones.end()));
	_int64 query_hash = (_int64)b.hash_value;
	query_hash = query_hash ^ (((_int64)target)<<33); // Target takes at most 9 bits
	if(want_capturable)
	{
		query_hash = query_hash ^ ((_int64)1)<<(33+9);
	}

	unordered_map<_int64, int>::iterator ii = hash_table.find(query_hash);

	if((want_capturable && result) || (!want_capturable && !result))
	{
		// Store a capture result
		hash_table[query_hash]=0;
	} else if(ii==hash_table.end() || (*ii).second < depth)
	{
		// Overwrite the previous (shallower) non-capture result if it's there
		hash_table[query_hash]=depth;
	}
}
// Updates hash value in place
void ZobristHash::add_stone(int pos, int colour, _int32& hash_value)
{
	if(colour==BLACK) hash_value=black_values[pos]^hash_value;
	else hash_value=white_values[pos]^hash_value;
}
void ZobristHash::remove_stone(int pos, int colour, _int32& hash_value)
{
	if(colour==BLACK) hash_value=black_values[pos]^hash_value;
	else hash_value=white_values[pos]^hash_value;
}