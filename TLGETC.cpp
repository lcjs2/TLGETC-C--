#include <iostream>
#include <vector>
#include <assert.h>
#include <utility>
#include "board.h"
#include "globals.h"
#include "reading.h"
#include "lifedeath.h"
#include "TLGETC.h"

using namespace std;



// The engine keeps its own copy of the board state, which has to be updated.
// For each lump, the engine stores a LumpData object, which contains
// - left, right, top, bottom coordinates
// - a pointer to the containing team(s)
// - lumps to which it is connected (later, upgrade this to a full Connection object)
// - notes on the lump's strategic importance
// - Its status (alive, unsettled, dead, unknown)
// The LumpData objects are stored in a map lump_data indexed by Lump pointer

// Each team is stored as a TeamData object, which contains
// - a list of lumps involved
// - Results from "alive-in-a-box"
// - Individual eyes
// - Space to extend
// - Potential connections to other alive groups
// - Any area surrounded by the group

// Remember that hash data is also part of the engine.


// Choose a move
int TLGETC::get_move()
{
	// When we reach this point, the engine has up-to-date connection data and a list of teams.
	// Some of the teams have some life-and-death data, but not all of it.
	cout<<"TLGETC - Determining move...\n";
	// Determine the status of each team by looking at the attached life-and-death data.
	// If a friendly group is unsettled, generate moves that strengthen it.
	// If an enemy group is unsettled, generate moves to attack it.

	// Generate moves in large empty areas of the board.

	// Generate moves that capture stones or extend for pure profit.



	return -1;
}


// Update board state, connection data, connected teams.
// Update life and death data for affected teams.
void TLGETC::make_move(int pos)
{
	assert(b.is_legal_move(pos));
	ReadingLog log;
	ReadingSettings settings;

	cout<< "TLGETC - Updating board state...\n";

	// Deal with pass separately
	if(pos==-1)
	{
		b.play_move(-1);
		return;
	}

	// Create list of adjacent lumps that need updating
	vector<Lump*> to_be_captured;
	vector<Lump*> to_be_merged;
	b.adj_lumps_captured(pos, HV, b.not_to_move(), to_be_captured);
	b.adj_lumps(pos, HV, b.to_move, to_be_merged);

	// IMPORTANT - to_be_merged contains pointers to lumps that don't exist. Any data referring to these has to be updated
	// (or, more likely, recalculated, since the lump at pos is interesting)

	// Make a list of interesting lumps (those that need updating).
	// - Anything adjacent to a captured group
	// -- If that previously had three or fewer liberties, anything adjacent to that (it just got new liberties, so maybe it can't be captured now)
	// The new group itself (added after playing move)
	// - Anything adjacent to the new group. (added afterwards)
	// -- If that now has three liberties or fewer, anything adjacent to that (maybe it can be captured now)
	// Anything within a certain distance of the new move (taxicab radius of 3 or 4).

	interesting_lumps.clear();
	for(vector<Lump*>::iterator ii = to_be_captured.begin();ii!=to_be_captured.end();ii++)
	{
		vector<Lump*> store; // lumps adjacent to this captured one.
		store.clear();
		b.lump_adj_lumps(*ii, HV, b.to_move, store);
		// Since they have new liberties, the status of their surrounding stones might change too.
		for(vector<Lump*>::iterator jj = store.begin();jj!=store.end();jj++)
		{
			if((*jj)->liberties<=3) b.lump_adj_lumps(*jj, HV, b.not_to_move(), interesting_lumps);
		}
		interesting_lumps.insert(interesting_lumps.end(), store.begin(), store.end());
	}
	// Remove to_be_captured and to_be_merged from interesting_lumps
	for(vector<Lump*>::iterator ii = interesting_lumps.begin();ii!=interesting_lumps.end();)
	{
		if(find(to_be_merged.begin(), to_be_merged.end(), *ii)!=to_be_merged.end() 
			|| find(to_be_captured.begin(), to_be_captured.end(), *ii)!=to_be_captured.end())
			ii=interesting_lumps.erase(ii);
		else ii++;
	}

	// Play move
	b.play_move(pos);
	interesting_lumps.push_back(b.board[pos]);

	// Lumps adjacent to new lump
	vector<Lump*> store2;
	store2.clear();
	b.lump_adj_lumps(b.board[pos], HV, b.to_move, store2);
	for(vector<Lump*>::iterator jj = store2.begin();jj!=store2.end();jj++)
	{ // Add this lump, and if it is now low on liberties, add its neighbours too.
		if((*jj)->liberties<=3) b.lump_adj_lumps(*jj, HV, b.to_move, interesting_lumps);
		interesting_lumps.push_back(*jj);
	}
	// Lumps in a taxicab radius of 3
	b.taxicab_radius_lumps(pos, 3, interesting_lumps);

	sort(interesting_lumps.begin(), interesting_lumps.end());
	interesting_lumps.erase(unique(interesting_lumps.begin(), interesting_lumps.end()),interesting_lumps.end());

	cout<<"TLGETC - Updating lump data...\n";
	
	//Add lump_data entry for new stone
	LumpData x(pos, b.board_size);
	lump_data.insert(make_pair(b.board[pos], x));
	// Now erase lump data of captured stones
	for(vector<Lump*>::iterator ii = to_be_captured.begin(); ii!=to_be_captured.end();ii++)
	{
		lump_data.erase(*ii);
	}
	// Anything in to_be_merged not equal to the lump at pos must have been merged,
	// so we merge its data.
	for(vector<Lump*>::iterator ii = to_be_merged.begin(); ii!=to_be_merged.end();ii++)
	{
		map<Lump*, LumpData>::iterator i=lump_data.find(b.board[pos]);
		map<Lump*, LumpData>::iterator j=lump_data.find(*ii);

		assert(i!=lump_data.end());
		i->second.left = min(i->second.left, j->second.left);
		i->second.right = max(i->second.right, j->second.right);
		i->second.top = min(i->second.top, j->second.top);
		i->second.bottom = max(i->second.bottom, j->second.bottom);
		lump_data.erase(*ii);
	}
	
	// Now update all the connection data, life and death, etc.

	cout<<"TLGETC - Recalculating connections...\n";

	// Create list of all lumps
	vector<Lump*> lump_list;
	lump_list.clear();
	b.all_lumps(lump_list);
	// Clear all old connection data involving interesting lumps
	for(vector<Lump*>::iterator ii = lump_list.begin();ii!=lump_list.end();ii++)
	{
		if(find(interesting_lumps.begin(), interesting_lumps.end(), *ii)!=interesting_lumps.end()) 
		{
			lump_data[*ii].connections.clear();
		}
		else
		{
			for(vector<Lump*>::iterator jj = lump_data[*ii].connections.begin();jj!=lump_data[*ii].connections.end();)
			{
				if(find(interesting_lumps.begin(), interesting_lumps.end(), *jj)!=interesting_lumps.end()
					|| find(to_be_merged.begin(), to_be_merged.end(), *jj)!=to_be_merged.end()) 
				{
						jj=lump_data[*ii].connections.erase(jj);
				}
				else jj++;
			}
		}

	}

	// Iterate through all pairs
	for(vector<Lump*>::iterator ii = lump_list.begin();ii!=lump_list.end()-1;ii++){
	for(vector<Lump*>::iterator jj = ii+1;jj!=lump_list.end();jj++)
	{
		// Call is_connected on each pair
		// If true, add each lump to the other's list of connections in lump_data
		if((*ii)->colour==(*jj)->colour 
			&& ((find(interesting_lumps.begin(), interesting_lumps.end(), *ii)!=interesting_lumps.end())
				|| (find(interesting_lumps.begin(), interesting_lumps.end(), *jj)!=interesting_lumps.end()))
			&& is_connected(*ii, *jj, settings, log))
		{
			lump_data.find(*ii)->second.connections.push_back(*jj);
			lump_data.find(*jj)->second.connections.push_back(*ii);
		}
	}}

	
	cout<<"TLGETC - Forming connected groups...\n";
	// Team objects created on the heap and their pointers stored in the vector teams
	// owned by the engine. Each lump has a pointer to the containing team in lump_data.
	// For now, just delete them...
	for(vector<Team*>::iterator ii = teams.begin(); ii!=teams.end();ii++)
	{
		delete *ii;
	}
	teams.clear();

	// ...and start over with new teams
	for(vector<Lump*>::iterator ii = lump_list.begin();ii!=lump_list.end();ii++)
	{
		Team* t = new Team(*ii, lump_data);
		lump_data[*ii].team = t;
		teams.push_back(t);
	}

	for(vector<Lump*>::iterator ii = lump_list.begin();ii!=lump_list.end();ii++)
	{
		for(vector<Lump*>::iterator jj=lump_data[*ii].connections.begin(); jj!=lump_data[*ii].connections.end();jj++)
		{
			Team* t1=lump_data.find(*ii)->second.team;
			assert(lump_data.find(*jj)!=lump_data.end());
			Team* t2=lump_data.find(*jj)->second.team;
			if(t1!=t2)
			{
				// Merge their teams into t1; every lump in t2 becomes a lump in t1, and we delete t2
				t1->l = min(t1->l, t2->l);
				t1->r = max(t1->r, t2->r);
				t1->t = min(t1->t, t2->t);
				t1->b = max(t1->b, t2->b);
				t1->size=t1->size + t2->size;

				t1->lumps.insert(t1->lumps.end(), t2->lumps.begin(), t2->lumps.end());
				for(vector<Lump*>::iterator kk = t2->lumps.begin();kk!=t2->lumps.end();kk++)
				{
					lump_data[*kk].team = t1;
				}

				teams.erase(find(teams.begin(), teams.end(), t2));
				delete t2;
			}
		}
	}

	// Now delete any life-and-death data that's out of date
	// (For now, it'll all be deleted)


	cout<<"TLGETC - Done.\n";

}

