#ifndef HASHING_H_120812
#define HASHING_H_120812

#include <vector>
#include <unordered_map>
#include <iostream>
#include "board.h"
class BoardState;

class HashObject
{
public:
	virtual int query_hash(BoardState& b, bool want_capturable, int target, int depth){return 0;}
	virtual void insert_hash(BoardState& b, bool want_capturable, int target, int depth, bool result){}
	virtual void add_stone(int pos, int colour, _int32& hash_value){};
	virtual void remove_stone(int pos, int colour, _int32& hash_value){};
};

class ZobristHash: public HashObject
{
public:
	std::vector<_int32> white_values;
	std::vector<_int32> black_values;
	std::unordered_map<_int64, int> hash_table;

	int queries;
	int positive_queries;

	ZobristHash(int board_size);
	~ZobristHash(){}

	void print_zobrist_values(void);

	int query_hash(BoardState& b, bool want_capturable, int target, int depth);
	void insert_hash(BoardState& b, bool want_capturable, int target, int depth, bool result);
	void add_stone(int pos, int colour, _int32& hash_value);
	void remove_stone(int pos, int colour, _int32& hash_value);
};

#endif