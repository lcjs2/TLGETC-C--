#ifndef ENGINE_H_060712
#define ENGINE_H_060712
#include "globals.h"
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include "board.h"
#include "hashing.h"


class Team;

// The engine stores a map of lumps and their corresponding LumpData objects
class LumpData
{
public:
	int left;
	int right;
	int top;
	int bottom;
	int status; // (ALIVE, DEAD, UNSETTLED, or -1 for unknown)
	// List of lumps connected to this one
	std::vector<Lump*> connections;

	// Team containing this lump
	Team* team;

	LumpData(int pos, int bsize)
	{
		left = right = pos % bsize;
		top = bottom = (int)(pos/bsize);
		status=-1;
	}
	LumpData(void){}
	std::string display(void)
	{
		std::stringstream str;
		str<< "L"<<left<<" R"<<right<<" T"<<top<<" B"<<bottom<<"\n";
		str<<"Connected to "<<connections.size()<<" other lumps" <<(connections.size()>0 ? ": " : ".");
		for(std::vector<Lump*>::iterator ii = connections.begin();ii!=connections.end();ii++)
		{
			str<<"("<<(*ii)->stones[0]%DEBUG_BS<<","<<(int)(*ii)->stones[0]/DEBUG_BS<< ") ";
		}
		str<<"\n";
		return str.str();
	}
};

// A result concerning the life and death of a team.
// Could be a life-in-a-box result, an eye, space to extend, or a potential connection
class LDResult
{
	// Valid as long as nothing changes in the following area
	int l;
	int r;
	int t;
	int b;
	// How many half eyes the result is worth
	int half_eyes;
};

// A team is a list of tactically connected lumps.
class Team
{
public:
	Team(Lump* target, std::map<Lump*, LumpData>& lump_data)
	{
		lumps.push_back(target);
		l=lump_data[target].left;
		r=lump_data[target].right;
		t=lump_data[target].top;
		b=lump_data[target].bottom;
		size=target->size();
	}

	std::vector<Lump*> lumps;
	int l;
	int r;
	int t;
	int b;

	int size;
};

// The engine object itself
class TLGETC: public Engine
{
public:
	BoardState b;
	std::map<Lump*, LumpData> lump_data;
	ZobristHash hash;

	std::vector<Lump*> interesting_lumps; // List of lumps affected by the most recent move

	std::vector<Team*> teams; // Keep a list of teams

	TLGETC(int bsize): Engine(bsize), b(bsize), hash(bsize) // Creates engine with empty board b of size bsize
	{
		b.hash = &hash;
	}
	int get_move();
	void make_move(int pos);
	std::string get_name(void){return "TLGETC v0.1";}

	// Processing methods *usually defined in other source files* (not TLGETC.cpp)
	bool is_connected(Lump* lump1, Lump* lump2, ReadingSettings settings, ReadingLog& log);

};

#endif