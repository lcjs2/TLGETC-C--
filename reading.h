#ifndef READING_H_030712
#define READING_H_030712

#include <vector>
#include <string>
#include <iostream>
#include <time.h>
#include <fstream>
#include <sstream>
#include "board.h"
#include "globals.h"
#include "hashing.h"

struct ReadingSettings
{
public:
	int reading_depth;
	int escape_libs;
	int branch_limit;
	HashObject* hash; // It is the responsibility of the engine to look after the hashing object

	ReadingSettings(void)
	{
		reading_depth=5;
		escape_libs=5;
		branch_limit=6;
	}
	ReadingSettings(int r, int e, int b)
	{
		reading_depth=r;
		escape_libs=e;
		branch_limit=b;
	}
};

class ReadingLog
{
public:
	time_t start_time;
	int current_depth;
	int total_moves;
	bool logging;
	std::ofstream log;
	//std::stringstream log;
	ReadingLog(void)
	{
		log.open("logfile.txt");
		current_depth=0; total_moves=0; logging=true;
	}
	void addline(std::string in)
	{
		if(logging==false) return;
		log<<"\n";
		for(int i=0;i<current_depth;i++){log<<"  ";}
		log<<in;
		//log.flush();
	}
	void addboard(BoardState& b)
	{
		if(logging==false) return;
			log << "\n\n  ";
	for(int i=0;i<b.board_size;i++){log << i%10;}
	log<<"\n +";
	for(int i=0;i<b.board_size;i++){log << "-";}
	log << "+  "<<(b.to_move==BLACK ? "Black" : "White") <<" to move\n";
	for(int j=0;j<b.board_size;j++)
	{
		log << j%10 << "|";
		for(int i=0;i<b.board_size;i++)
		{
			if(b.board[(b.board_size*j)+i]==NULL)
			{
				log<<(b.ko_marker==(b.board_size*j)+i ? ":" : ".");
			} else {
				if(b.board[(b.board_size*j)+i]->colour==BLACK)
				{
					log<<(b.board[(b.board_size*j)+i]->invincible ? "k" : "X");
				} else {
					log<<(b.board[(b.board_size*j)+i]->invincible ? "c" : "O");
				}
			}
		}
		log<<"|\n";
	}
	log <<" +";
	for(int i=0;i<b.board_size;i++){log << "-";}
	log<< "+\n";
	//log.flush();
	}
	void add(std::string in) {if(logging){log<<in;}}
	void addpos(int in) {if(logging){log<<"("<<(in%9)<<" "<<(int)(in/9)<<")"; start_time = time(NULL);}}
	void addint(int in) {if(logging){log<<in;}}
	void pause(void){int i; std::cout<<"\nPaused..."; std::cin>>i; return;}

};
// Interface functions - call these! In particular, they correct for colour.

// get_status returns ALIVE, DEAD, UNSETTLED for target.
// ALIVE = cannot be captured; UNSETTLED = can be captured, but capture can be prevented if target moves first; DEAD = no escape.
// If get_defences is false, can only return ALIVE or DEAD. Outputs to capture_here and escape_here, unless they are NULL.
int get_status(BoardState& b, int target, bool get_defences, std::vector<int>* capture_here, std::vector<int>* escape_here, ReadingSettings& settings, ReadingLog& log);

// Essentially colour-correcting versions of can_capture and can_escape
bool get_capturable(BoardState& b, int target, int colour, ReadingSettings& settings, ReadingLog& log);
bool get_escapable(BoardState& b, int target, ReadingSettings& settings, ReadingLog& log);

//Returns true if target can be captured with a series of ataris
bool is_ladderable(BoardState& b, Lump& target, std::vector<int>* output, ReadingLog& log);
bool is_ladderable(BoardState& b, int pos, std::vector<int>* output, ReadingLog& log);
bool is_laddered(BoardState& bd, int pos, ReadingLog& log);

bool can_capture(BoardState& b, int target, int depth, ReadingSettings& settings, ReadingLog& log);
bool can_escape(BoardState& b, int target, int depth, ReadingSettings& settings, ReadingLog& log);
void capture_moves(BoardState& b, int target, int depth, std::vector<int>& output, ReadingLog& log);
void escape_moves(BoardState& b, int target, int depth, std::vector<int>& output, ReadingLog& log);


#endif