#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <assert.h>
#include <algorithm>
#include "board.h"
#include "lifedeath.h"
#include "globals.h"
#include "reading.h"


using namespace std;


string EyeGraph::display(void)
{
	stringstream ss;
	ss << "Eyespace";
	ss<<"\nSpaces: ";
	for(vector<int>::iterator ii=spaces.begin();ii!=spaces.end();ii++)
	{
		ss<<" ("<<(*ii)%DEBUG_BS<<","<<(int)(*ii/DEBUG_BS)<<")";
	}
	ss<<"\nAdj spaces: ";
	for(vector<int>::iterator ii=adj_spaces.begin();ii!=adj_spaces.end();ii++)
	{
		ss<<" ("<<(*ii)%DEBUG_BS<<","<<(int)(*ii/DEBUG_BS)<<")";
	}
	ss<<"\nDead stones:";
	for(vector<vector<int> >::iterator ii=dead_stones.begin();ii!=dead_stones.end();ii++)
	{
		ss<<"\n";
		for(vector<int>::iterator jj=ii->begin(); jj!=ii->end();jj++)
		{
			ss<<" ("<<(*jj)%DEBUG_BS<<","<<(int)(*jj/DEBUG_BS)<<") ";
		}
	}
	ss<<"\nCapturable stones:";
	for(vector<vector<int> >::iterator ii=killable_stones.begin();ii!=killable_stones.end();ii++)
	{
		ss<<"\n";
		for(vector<int>::iterator jj=ii->begin(); jj!=ii->end();jj++)
		{
			ss<<" ("<<(*jj)%DEBUG_BS<<","<<(int)(*jj/DEBUG_BS)<<") ";
		}
	}
	ss<<"\nUndetermined enemy stones:";
	for(vector<vector<int> >::iterator ii=all_enemy_stones.begin();ii!=all_enemy_stones.end();ii++)
	{
		ss<<"\n";
		for(vector<int>::iterator jj=ii->begin(); jj!=ii->end();jj++)
		{
			ss<<" ("<<(*jj)%DEBUG_BS<<","<<(int)(*jj/DEBUG_BS)<<") ";
		}
	}

	return ss.str();	
}


// Returns true if the specified group is uncapturable even if we pass continuously
bool is_pass_alive(BoardState& b, int pos, vector<int>& codependants, ReadingLog& log)
{
	// Get all liberties
	if(b.board[pos]->invincible) return true;
	vector<int> liberties;
	vector<int> illegal_moves;
	vector<Lump*> pcodependants;
	b.lump_adj_liberties(pos, HV, liberties);

	BoardState b_copy = b;
	b_copy.ko_marker=-1;
	
	if(b_copy.to_move==b_copy.colour(pos)) b_copy.play_move(-1); //Keep capturer to move
	while(!liberties.empty())
	{
		int next_lib = liberties.back();
		if(b_copy.is_legal_move(next_lib))
		{
			b_copy.play_move(next_lib);
			b_copy.play_move(-1);
		} else {
			illegal_moves.push_back(next_lib);
		}
		liberties.pop_back();
	}

	if(b_copy.board[pos]==NULL || b_copy.liberties(pos)==1) return false;

	// Now we have a list of liberties which cannot be filled. We can only play them if we can remove one of their surrounding groups.
	// For each illegal move liberty, we find a list of all neighbouring lumps of the same colour, which are
	// the ones making the move illegal. Then we create a list of all friendly lumps neighbouring these lumps 
	// and the liberty itself. The liberty remains unremovable if all of these friendly lumps are also pass-alive.
	// Codependants are assumed to be pass-alive.

	// Create list of codependant lumps (including the target itself)
	for(vector<int>::iterator ii = codependants.begin(); ii!=codependants.end();ii++)
	{
		if(b_copy.board[*ii]!=NULL) pcodependants.push_back(b_copy.board[*ii]);
	}
	pcodependants.push_back(b_copy.board[pos]);
	// Make our own copy of codependants list, and add the target group
	vector<int> store_codeps = codependants;
	store_codeps.push_back(pos);

	assert(illegal_moves.size()>1);
	vector<Lump*> capturers; // the capturing lumps surrounding the liberty
	vector<Lump*> friends; // the friendly stones surrounding these and the liberty itself
	vector<int> unremovables; // a store for the definitely unplayable ones
	for(vector<int>::iterator ii = illegal_moves.begin();ii!=illegal_moves.end();ii++)
	{
		capturers.clear();
		friends.clear();
		b_copy.adj_lumps(*ii, HV, FRIEND, capturers);
		for(vector<Lump*>::iterator jj=capturers.begin(); jj!=capturers.end();jj++)
		{
			b_copy.lump_adj_lumps(*jj, HV, ENEMY, friends);
		}
		b_copy.adj_lumps(*ii, HV, ENEMY, friends);
		sort(friends.begin(), friends.end());
		friends.erase(unique(friends.begin(), friends.end()), friends.end());

		// Now friends is a list of lumps which must be pass-alive
		for(vector<Lump*>::iterator jj=friends.begin(); jj!=friends.end();jj++)
		{
			if(find(pcodependants.begin(),pcodependants.end(),*jj)!=pcodependants.end()) continue;
			if(!is_pass_alive(b_copy, (*jj)->stones[0], store_codeps, log))return false;
		}		
	}
	return true;

}

// Returns true if group is effectively pass-alive
bool is_epa(BoardState& b, int pos, ReadingSettings& settings, ReadingLog& log)
{

	return false;
}

// Fills in everything on or outside border (and in extra_boarder) with invincible enemy stones, 
// and returns ALIVE if we are unable to capture anything in targets. 
// Returns DEAD if something can be captured whatever the defence
// Returns UNSETTLED if it depends who moves first.
// Any friendly lumps with no liberties in the box are completely replaced by enemy stones.
int get_status_in_box(BoardState& bd, vector<int> targets, int left, int right, int top, int bottom, vector<int>& extra_border, ReadingLog& log)
{
	assert(targets.size()>0);
	log.addline("Calling get_status_in_box on this position:");
	log.addboard(bd);
	// Make explicit list of invincible stones to be added
	vector<int> inv;
	for(int i=0; i<bd.board_size;i++)
	{
		for(int j=0; j<bd.board_size;j++)
		{
			if(i<=left || i>=right || j<=top || j>=bottom) inv.push_back(i + (j*bd.board_size));
		}
	}
	// Add the specified extras
	inv.insert(inv.end(), extra_border.begin(), extra_border.end());
	// Add any friendly groups with no liberties inside the box
	vector<Lump*> store;
	for(int i = left+1;i<right;i++)
	{
		for(int j = top+1;j<bottom;j++)
		{
			if(bd.board[i + j*bd.board_size]!=NULL)
			{
				if(!bd.has_libs_in_box(i + j*bd.board_size, left, right, top, bottom, extra_border)) store.push_back(bd.board[i + j*bd.board_size]);
			}
		}
	}
	sort(store.begin(), store.end());
	store.erase(unique(store.begin(), store.end()), store.end());
	// Now store contains a list of groups with no liberties in box. Add them to invincible stones.
	for(vector<Lump*>::iterator ii=store.begin(); ii!=store.end();ii++)
	{
		inv.insert(inv.end(), (*ii)->stones.begin(), (*ii)->stones.end());
	}
	sort(inv.begin(), inv.end());
	inv.erase(unique(inv.begin(), inv.end()), inv.end());

	//Create new board with invincible stones on it
	BoardState b(bd.board_size);
	Lump* inv_lump = new Lump; // Will be destroyed with b, which is on the stack
	int colour = bd.colour(targets[0]);
	inv_lump->colour = bd.interpret_other_colour(colour);
	inv_lump->liberties = 100;
	inv_lump->stones=inv;
	inv_lump->invincible=true;
	for(vector<int>::iterator jj=inv_lump->stones.begin();jj!=inv_lump->stones.end();jj++) b.board[*jj]=inv_lump;

	// Fill the box interior, and create list of empty spaces inside.
	vector<int> eyespace;
	for(int i = left+1;i<right;i++)
	{
		for(int j = top+1;j<bottom;j++)
		{
			int pos = i + j*b.board_size;
			if(b.board[pos]==NULL && bd.board[pos]!=NULL)
			{
				if(b.to_move!=bd.colour(pos)) b.play_move(-1);
				b.play_move(pos);
			}
		}
	}

	log.addline("After filling in invincible stones:");
	log.addboard(b);

	
	EyeGraph output;
	log.addline("Computing eyespace...");
	box_compute_eyespace(b, left, right, top, bottom, colour, output, log);
	log.addline(output.display());

	vector<int> vital_points;
	int status = eval_eyespace_I(output, b.board_size, vital_points);
	log.addline("Initial evaluation of eyespace returned: ");
	log.addint(status);
	// If eval I returns dead, then we are certainly dead.
	if(status==DEAD) return DEAD;

	//For now...
	return status;


}

// Compute the empty spaces in the box that could be eyespaces
// Only list enemy stones in the region - don't determine status
// Colour is colour of target
void box_compute_eyespace(BoardState& b, int left, int right, int top, int bottom, int colour, EyeGraph& output, ReadingLog& log)
{
	log.addline("Starting BCE");
	// Make list of potential eyespace
	for(int i = left+1;i<right;i++)
	{
		for(int j = top+1;j<bottom;j++)
		{
			int pos = i + j*b.board_size;
			if(b.board[pos]!=NULL) continue;
			// Discard definite false eyes
			vector<int> store;
			b.adj_points(pos, DIAG, store);
			int total=0;
			if(store.size()<4) total++;
			for(vector<int>::iterator ii=store.begin();ii!=store.end();ii++)
			{
				// If two diagonal invincible stones (one on edge) then discard
				if(b.board[*ii]!=NULL && b.colour(*ii)!=colour && b.board[*ii]->invincible) total++;
			}
			if(total>=2) continue;
			// Add to adj_spaces
			store.clear();
			b.adj_points(pos, HV, store);
			bool flag=false;
			for(vector<int>::iterator ii=store.begin();ii!=store.end();ii++)
			{
				// If adjacent to a w stone, we don't want it
				if(b.board[*ii]!=NULL && b.colour(*ii)!=colour) 
				{
					output.adj_spaces.push_back(pos);
					flag=true;
					break;
				}
			}
			if(flag) continue;
			// If it's not adjacent to a w stone or a false eye, add to possible spaces
			output.spaces.push_back(pos);					
		}
	}
	// Now eyespace contains all liberties where we could possibly end up with an eye
	// Add all non-invincible enemy stones in the box (finding their status is expensive)
	vector<Lump*> all_lumps;
	b.all_lumps(all_lumps);
	for(vector<Lump*>::iterator ii=all_lumps.begin();ii!=all_lumps.end();ii++)
	{
		if((*ii)->colour!=colour && !(*ii)->invincible)
		{
			output.all_enemy_stones.push_back((*ii)->stones);
		}
	}

	
}



// Evaluate whether a given eyespace could possibly make life.
// Takes eyegraph object with the status of enemy stones *not* evaluated.
// Hopes to return a quick DEAD status, or UNSETTLED with a small list of possible life moves
// If returns DEAD, definitely dead (within box, of course).
// If returns UNSETTLED, output contains all moves that could possible live (and they are definite killing moves).
// If unsure, returns ALIVE.
// Eyespace contains: empty_spaces, adj_spaces, all_enemy_stones
int eval_eyespace_I(EyeGraph& eyespace, int board_size, vector<int>& output)
{
	int total = eyespace.adj_spaces.size() + eyespace.spaces.size()+eyespace.all_enemy_stones.size();
	if(total>=7) return ALIVE;
	if(total<=1) return DEAD;

	//Suppose first that all spaces are enemy stones or adjacent to same
	if(eyespace.spaces.size()==0)
	{ 
		// If two enemy lumps, it's going to depend very much on whether they're capturable
		if(eyespace.all_enemy_stones.size()>1) return UNSETTLED;
		// If one enemy lump of one or two stones, return DEAD
		if(eyespace.all_enemy_stones[0].size()<=2) return DEAD;
		// If one enemy lump of at least three stones, seki is a possibility
		return UNSETTLED;
	}

	// Now calculate vertex order for each point in the set
	vector<int> neighbours;
	vector<int> all_spaces;
	all_spaces.insert(all_spaces.end(), eyespace.spaces.begin(), eyespace.spaces.end());
	all_spaces.insert(all_spaces.end(), eyespace.adj_spaces.begin(), eyespace.adj_spaces.end());
	for(vector<vector<int> >::iterator ii = eyespace.all_enemy_stones.begin();ii!=eyespace.all_enemy_stones.end();ii++)
	{
		all_spaces.insert(all_spaces.end(), (*ii).begin(), (*ii).end());
	}
	for(vector<int>::iterator ii = all_spaces.begin();ii!=all_spaces.end();ii++)
	{
		neighbours.push_back((*ii)-1); neighbours.push_back((*ii)+1); neighbours.push_back((*ii)-board_size); neighbours.push_back((*ii)+board_size);
	}
	for(vector<int>::iterator ii = all_spaces.begin();ii!=all_spaces.end();ii++)
	{
		switch(count(neighbours.begin(), neighbours.end(), *ii))
		{
		case 0: eyespace.zero_nb.push_back(*ii); break;
		case 1: eyespace.one_nb.push_back(*ii); break;
		case 2: eyespace.two_nb.push_back(*ii); break;
		case 3: eyespace.three_nb.push_back(*ii); break;
		case 4: eyespace.four_nb.push_back(*ii); break;
		}
	}
	// If we have any points with no neighbours, return ALIVE (since we have at last two points)
	if(eyespace.zero_nb.size()>0) return ALIVE;
	// Now we have stored each eye space according to its number of neighbours.
	// We use this to determine what shape we have.
	int counter=0;
	vector<int> store;
	int an_int;
	vector<int> list_all_stones;
	for(vector<vector<int> >::iterator ii = eyespace.all_enemy_stones.begin(); ii!=eyespace.all_enemy_stones.end();ii++)
	{
		list_all_stones.insert(list_all_stones.end(), (*ii).begin(), (*ii).end());
	}
	switch(eyespace.one_nb.size()
		+ 10*eyespace.two_nb.size()
		+ 100*eyespace.three_nb.size()
		+ 1000*eyespace.four_nb.size())
	{
	case 2: // 2 one-nb = line of two
		return DEAD;
	case 12: // 1 two-nb, 2 one-nb = line of three
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.two_nb[0])!=list_all_stones.end()) return DEAD;
		eyespace.vital_points.push_back(eyespace.two_nb[0]);
		return UNSETTLED;
	case 40: // 4 two_nb = square four
		return DEAD;
	case 22: // 2 two_nb, 2 one_nb = line of four
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.two_nb[0])!=list_all_stones.end()) counter++;
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.two_nb[1])!=list_all_stones.end()) counter++;
		if(counter==2) {return DEAD;}
		else if(counter==0) {return ALIVE;}
		else
		{eyespace.vital_points.push_back(eyespace.two_nb[0]);eyespace.vital_points.push_back(eyespace.two_nb[1]); return UNSETTLED;}
	case 103: // 1 three_nb, 3 one_nb = T shape
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.three_nb[0])!=list_all_stones.end()) return DEAD;
		eyespace.vital_points.push_back(eyespace.three_nb[0]);
		return UNSETTLED;
	case 4: // 4 one_nb = two separate lines of two
		return ALIVE;
	case 32: // 3 two_nb, 2 one_nb = line of five
		return ALIVE;
	case 113: // 1 three_nb, 1 two_nb, 3 one_nb = extended T shape of five
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.three_nb[0])!=list_all_stones.end()) counter++;
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.two_nb[0])!=list_all_stones.end()) counter++;
		if(counter==2) {return DEAD;}
		else if(counter==0) {return ALIVE;}
		else
		{eyespace.vital_points.push_back(eyespace.two_nb[0]);eyespace.vital_points.push_back(eyespace.three_nb[0]); return UNSETTLED;}
	case 131: // 1 three_nb, 3 two_nb, 1 one_nb = bulky five
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.three_nb[0])!=list_all_stones.end()) return DEAD;
		eyespace.vital_points.push_back(eyespace.three_nb[0]);
		return UNSETTLED;
	case 1004: // 1 four_nb, 4 one_nb = + shape
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.four_nb[0])!=list_all_stones.end()) return DEAD;
		eyespace.vital_points.push_back(eyespace.four_nb[0]);
		return UNSETTLED;
	case 14: // 1 tqwo_nb, 4 one_nb = line of three and separate line of two
		return ALIVE;
	case 42: // 4 two_nb, 2 one_nb = line of six
		return ALIVE;
	case 123: // 1 three_nb, 2 two_nb, 3 one_nb = T shape extended twice along one arm OR extended on two different arms
		return ALIVE;
	case 1014: // 1 four_nb, 1 two_nb, 4 one_nb = + shape extended on one arm
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.four_nb[0])!=list_all_stones.end()) counter++;
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.two_nb[0])!=list_all_stones.end()) counter++;
		if(counter==2) {return DEAD;}
		else if(counter==0) {return ALIVE;}
		else
		{eyespace.vital_points.push_back(eyespace.two_nb[0]);eyespace.vital_points.push_back(eyespace.four_nb[0]); return UNSETTLED;}
	case 204: // 2 three_nb, 4 one_nb = line of four with two extensions, one to each middle space
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.three_nb[0])!=list_all_stones.end()) counter++;
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.three_nb[1])!=list_all_stones.end()) counter++;
		if(counter==2) {return DEAD;}
		else if(counter==0) {return ALIVE;}
		else
		{eyespace.vital_points.push_back(eyespace.three_nb[0]);eyespace.vital_points.push_back(eyespace.three_nb[1]); return UNSETTLED;}
	case 240: // 2 three_nb, 4 two_nb = rectangle of six
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.three_nb[0])!=list_all_stones.end()) counter++;
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.three_nb[1])!=list_all_stones.end()) counter++;
		if(counter==2) {return DEAD;}
		else if(counter==0) {return ALIVE;}
		else
		{eyespace.vital_points.push_back(eyespace.three_nb[0]);eyespace.vital_points.push_back(eyespace.three_nb[1]); return UNSETTLED;}
	case 1032: // 1 four_nb, 3 two_nb, 2 one_nb = rabbit six
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.four_nb[0])!=list_all_stones.end()) return DEAD;
		eyespace.vital_points.push_back(eyespace.four_nb[0]);
		return UNSETTLED;
	case 222: // 2 three_nb, 2 two_nb, 2 one_nb = bulky five extended on one of the back corners. This case is an actual collision. 
		// If the three_nb points are adjacent then they are vital points. If they are not, then we could still have seki even with both filled.
		an_int = eyespace.three_nb[0]-eyespace.three_nb[1];
		if(an_int==1 || an_int==-1 || an_int==board_size || an_int==-board_size)
		{
			if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.three_nb[0])!=list_all_stones.end()) counter++;
			if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.three_nb[1])!=list_all_stones.end()) counter++;
			if(counter==2) {return DEAD;}
			else if(counter==0) {return ALIVE;}
			else
			{eyespace.vital_points.push_back(eyespace.three_nb[0]);eyespace.vital_points.push_back(eyespace.three_nb[1]); return UNSETTLED;}
		} else {
			return ALIVE;
		}
	case 141: // 1 three_nb, 4 two_nb, 1 one_nb = bulky five extended along arm. Only one of the two_nb is a vital point.
		store.push_back(eyespace.one_nb[0]+1);store.push_back(eyespace.one_nb[0]-1);store.push_back(eyespace.one_nb[0]+board_size);store.push_back(eyespace.one_nb[0]-board_size);
		store.insert(store.end(), eyespace.two_nb.begin(), eyespace.two_nb.end());
		sort(store.begin(), store.end());
		an_int = *(adjacent_find(store.begin(), store.end()));
		if(find(list_all_stones.begin(), list_all_stones.end(), eyespace.three_nb[0])!=list_all_stones.end()) counter++;
		if(find(list_all_stones.begin(), list_all_stones.end(), an_int)!=list_all_stones.end()) counter++;
		if(counter==2) {return DEAD;}
		else if(counter==0) {return ALIVE;}
		else
		{eyespace.vital_points.push_back(eyespace.three_nb[0]);eyespace.vital_points.push_back(an_int); return UNSETTLED;}
	case 24: // 2 two_nb, four one_nb = line of four and line of two OR two separate lines of three
		return ALIVE;
	case 6: // 6 one_nb = three separate lines of two
		return ALIVE;
	default:
		assert(false);
		return ALIVE;
	}
}



// Code to get status of enemy stones in box (expensive):
/*			int status;
			log.addline("Calling get_status on "); log.addpos((*ii)->stones[0]);
			status=get_status(b, (*ii)->stones[0], true, NULL, NULL, stgs, log);
			log.addline("...get_status returned "); log.addint(status);
			if(status==DEAD)
			{
				output.dead_stones.push_back((*ii)->stones);
			} else if(status==UNSETTLED)
			{
				output.killable_stones.push_back((*ii)->stones);
			}*/