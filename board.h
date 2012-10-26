#ifndef BOARD_H_300712
#define BOARD_H_300712

#include <vector>
#include "globals.h"
#include "hashing.h"
class HashObject;

class Lump
{
public:
	Lump(int, int);
	Lump(void){}

	std::vector<int> stones;
	int index;
	int colour;
	int liberties;
	bool invincible;
	void add_stone(int);
	int size(void){return stones.size();}
	std::string display();
};
		
class BoardState
{
public:
	std::vector<Lump*> board;
public:
	BoardState(int); // Constructor with board size - returns empty board.
	BoardState(const BoardState& other); // Copy constructor
	~BoardState(void); // Destructor must delete all lump objects on the heap
	int board_size;
	int to_move;
	int not_to_move(void){return interpret_other_colour(to_move);}
	int ko_marker;

	// Hash object is owned by engine. It contains the Zobrist hash values
	// and methods to update the hash in place.
	HashObject* hash;
	_int32 hash_value;

	void display(void);
	bool is_legal_move(int);
	int delete_lump(Lump*);
	void merge_two_lumps(Lump& lump1, Lump& lump2);
	int recalculate_liberties(Lump& lump);
	void play_move(int);
	
	// Utility functions
	// Returns colour relative to board state
	int interpret_colour(int colour);
	int interpret_other_colour(int colour);

	// A mountain of functions that return data about the board position
	int colour(int pos){if(board[pos]==NULL){return -1;}else{return board[pos]->colour;}}
	int liberties(int pos){return board[pos]->liberties;}

	void taxicab_radius(int pos, int r, std::vector<int>& output);
	void taxicab_radius_lumps(int pos, int r, std::vector<Lump*>& output);

	void adj_points(int pos, int directions, std::vector<int>& output);
	void adj_liberties(int pos, int directions, std::vector<int>& output);
	int n_adj_liberties(int pos, int directions);
	void lump_adj_points(Lump* target, int directions, std::vector<int>& output);
	void lump_adj_points(int pos, int directions, std::vector<int>& output);
	void lump_adj_liberties(Lump* target, int directions, std::vector<int>& output);
	void lump_adj_liberties(int pos, int directions, std::vector<int>& output);

	int lump_n_adj_liberties(int pos, int directions);
	int lump_n_adj_liberties(Lump* target, int directions);
	int resulting_liberties(int pos, int colour);
	bool is_atari(int pos, int colour);
	bool is_capture(int pos, int colour);

	void all_lumps(std::vector<Lump*>& output);
	void adj_lumps(int pos, int directions, int colour, std::vector<Lump*>& output);
	void adj_lumps_captured(int pos, int directions, int colour, std::vector<Lump*>& output);
	void lump_adj_lumps(int pos, int directions, int colour, std::vector<Lump*>& output);
	void lump_adj_lumps(Lump* target, int directions, int colour, std::vector<Lump*>& output);
	void adj_lump_pos(int pos, int directions, int colour, std::vector<int>& output);

	bool is_adj_to(int pos, int directions, int colour);

	bool has_libs_in_box(int target, int left, int right, int top, int bottom, std::vector<int>& extra_border);

private:
	BoardState& operator=(const BoardState&); // No assignment operator
	
};//class board_state





#endif