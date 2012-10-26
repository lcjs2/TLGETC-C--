#include <string>
#include <algorithm>
#include <vector>
#include "globals.h"
#include "reading.h"
#include "board.h"
#include "connections.h"
#include "TLGETC.h"

using namespace std;

// Engine method that returns true if two lumps are tactically connected
bool TLGETC::is_connected(Lump* lump1, Lump* lump2, ReadingSettings settings, ReadingLog& log)
{
	log.addline("Determining connection between ");
	log.addpos(lump1->stones[0]); log.add(" and ");log.addpos(lump2->stones[0]);
	if(lump1->liberties==1 || lump2->liberties==1) return false;
	int l1=lump_data.find(lump1)->second.left; int l2=lump_data.find(lump2)->second.left;
	int r1=lump_data.find(lump1)->second.right; int r2=lump_data.find(lump2)->second.right;
	int t1=lump_data.find(lump1)->second.top; int t2=lump_data.find(lump2)->second.top;
	int b1=lump_data.find(lump1)->second.bottom; int b2=lump_data.find(lump2)->second.bottom;
	if(l1>r2+2 || l2>r1+2 || t1>b2+2 || t2>b1+2) return false;

	vector<int> store;
	vector<int> shared_libs;
	b.lump_adj_liberties(lump1, HV, store);
	b.lump_adj_liberties(lump2, HV, store);
	sort(store.begin(), store.end());
	vector<int>::iterator ii = adjacent_find(store.begin(), store.end());
	while(ii!=store.end())
	{
		shared_libs.push_back(*ii);
		ii=adjacent_find(ii+1, store.end());
	}
	if(shared_libs.size()>=2) return true;
	// If we have one shared liberty, quick check: is this playing one stone into hanging connection?
	if(shared_libs.size()==1 && (b.n_adj_liberties(shared_libs[0], HV)<=1) 
		&& !b.is_adj_to(shared_libs[0], HV, b.interpret_other_colour(lump1->colour))
		&& !b.is_capture(shared_libs[0], b.interpret_other_colour(lump1->colour))) return true;

	// Now see if the lumps share any common enemy groups that can be captured
	vector<Lump*> store2;
	vector<Lump*> shared_lumps;
	b.lump_adj_lumps(lump1, HV, b.interpret_other_colour(lump1->colour), store2);
	b.lump_adj_lumps(lump2, HV, b.interpret_other_colour(lump1->colour), store2);
	sort(store2.begin(), store2.end());
	vector<Lump*>::iterator jj = adjacent_find(store2.begin(), store2.end());
	while(jj!=store2.end())
	{
		int status = get_status(b, (*jj)->stones[0], true, NULL, NULL, settings, log);
		if(status==DEAD) return true;
		if(status==UNSETTLED) shared_lumps.push_back(*jj);
		jj=adjacent_find(jj+1, store2.end());
	}
	if(shared_lumps.size() + shared_libs.size() >=2) return true;

	// If we have one cutting point, check whether it's capturable
	if(shared_libs.size()==1)
	{
		int sl=shared_libs[0];
		BoardState b_copy=b;
		if(b_copy.to_move==lump1->colour) b_copy.play_move(-1);
		if(b_copy.is_legal_move(sl))
		{
			b_copy.play_move(sl);
			if(get_capturable(b_copy, sl, -1, settings, log)) return true;
			// So not capturable.
			// Now test for one-point jump connection
			// If not on edge...
			if(sl>=b_copy.board_size && sl<(b_copy.board_size * (b_copy.board_size-1))&& sl%b_copy.board_size != 0 && sl%b_copy.board_size != b_copy.board_size-1)
			{
				vector<int> store;
				store.clear();
				b_copy.adj_liberties(sl, HV, store);
				// We have pushed; now try blocking each side
				for(vector<int>::iterator ii = store.begin();ii!=store.end();ii++)
				{
					if(b_copy.is_legal_move(*ii))
					{
						BoardState b_copy2 = b_copy;
						b_copy2.play_move(*ii);
						// We have pushed and blocked; now compute two diagonal moves
						int m1, m2;
						if((*ii)-sl==1 || (*ii)-sl==-1)
						{
							m1=(*ii)-b_copy2.board_size;
							m2=(*ii)+b_copy2.board_size;
						} else {
							m1=(*ii)-1;
							m2=(*ii)+1;
						}
						// Checks now depend on whether the pushing stone is in atari.
						// If it's capturable, then play first escaping move and return true if either m1 or m2 is capturable.
						// (We assume that if one side is capturable, then we can defend by playing the other.)
						// If it's not: play m1 for opponent, m2 for self (or pass) and see if m1 is capturable.
						//   If so, return true if m2 is also capturable from original. (In this case we are connected by 
						//   capturing/defending m2 and letting m1 die)
						// Repeat with m1, m2 reversed.
						vector<int> escs;
						int status = get_status(b_copy2, sl, true, NULL, &escs, settings, log);
						if(status==UNSETTLED)
						{
							if(b.colour(m1)==lump1->colour || b.colour(m2)==lump1->colour) return true;
							// We only try escs[0]; could try all of them if we're less lazy
							b_copy2.play_move(escs[0]);							
							if(get_capturable(b_copy2, m1, b_copy2.colour(sl), settings, log)) return true;
							if(get_capturable(b_copy2, m2, b_copy2.colour(sl), settings, log)) return true;
						}
						if(status==ALIVE)
						{
							if(b_copy2.is_legal_move(m1))
							{
								BoardState bc3 = b_copy2;
								bc3.play_move(m1);
								if(bc3.is_legal_move(m2)) bc3.play_move(m2);
								else bc3.play_move(-1);
								if(get_status(bc3, m1, true, NULL, NULL, settings, log)==DEAD
									&& get_capturable(b_copy2, m2, b_copy2.colour(sl), settings, log)) return true;
							} else {
								if(get_capturable(b_copy2, m2, b_copy2.colour(sl), settings, log)) return true;
							}
							if(b_copy2.is_legal_move(m2))
							{
								BoardState bc4 = b_copy2;
								bc4.play_move(m2);
								if(bc4.is_legal_move(m1)) bc4.play_move(m1);
								else bc4.play_move(-1);
								if(get_status(bc4, m2, true, NULL, NULL, settings, log)==DEAD
									&& get_capturable(b_copy2, m1, b_copy2.colour(sl), settings, log)) return true;
							} else {
								if(get_capturable(b_copy2, m1, b_copy2.colour(sl), settings, log)) return true;
							}
						}

					}
				}
			}
		}
	}



	return false;
}
