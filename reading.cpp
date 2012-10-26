#include <iostream>
#include <assert.h>
#include <algorithm>
#include <map>
#include <utility>
#include <vector>
#include <string>
#include <assert.h>
#include "board.h"
#include "globals.h"
#include "reading.h"

using namespace std;

// Gets status of target group. Returns ALIVE, DEAD or UNSETTLED and outputs to vectors as necessary.
// If get_defences is false, no escape moves will be produced (and result will be ALIVE or DEAD)
// If either output pointer is NULL, nothing will be sent there and the function will return as soon as
// it can be sure of an ALIVE/DEAD result
int get_status(BoardState& b, int target, bool get_defences, vector<int>* capture_here, vector<int>* escape_here, ReadingSettings& settings, ReadingLog& log)
{
	assert(b.board[target]!=NULL);
	log.addline("Calling get_status on "); log.addpos(target);
	if(b.board[target]->invincible) return ALIVE;
	bool something_captures=false;
	bool something_escapes=false;
	vector<int> store;
	BoardState b_copy = b;
	// Insert pass if we're trying to capture our own stones
	if(b_copy.to_move==b_copy.colour(target)){b_copy.play_move(-1);}
	// Can escape early if ladderable and we don't want to record capturing moves
	if(capture_here==NULL && is_ladderable(b, target, NULL, log))
	{
		something_captures=true;
	} else {
		capture_moves(b_copy, target, settings.reading_depth, store, log);
		for(vector<int>::iterator ii=store.begin();ii!=store.end();ii++)
		{
			BoardState b_copy2 = b_copy;
			b_copy2.play_move(*ii);
			if(b_copy2.board[target]==NULL || can_escape(b_copy2, target, settings.reading_depth, settings, log)==false)
			{
				if(capture_here==NULL)
				{
					if(get_defences) {something_captures=true; break;}
					else return DEAD;
				} else {
					capture_here->push_back(*ii);
					something_captures=true;
				}
			}
		}
	}

	if(something_captures==false) 
	{
		return ALIVE;
	}
	if(get_defences==false) 
	{
		return DEAD;
	}

	store.clear();
	b_copy.play_move(-1);
	escape_moves(b_copy, target, settings.reading_depth, store, log);
	
	for(vector<int>::iterator ii=store.begin();ii!=store.end();ii++)
	{
		BoardState b_copy2 = b_copy;
		b_copy2.play_move(*ii);
		if(can_capture(b_copy2, target, settings.reading_depth, settings, log)==false)
		{
			if(escape_here==NULL)
			{
				return UNSETTLED;
			} else {					
				escape_here->push_back(*ii);
				something_escapes=true;
			}
		}
	}

	if(something_escapes) return UNSETTLED;
	else return DEAD;
}
// If you pass an empty space as target, will play there with colour and then check.
// Returns true if illegal move.
bool get_capturable(BoardState& b, int target, int colour, ReadingSettings& settings, ReadingLog& log)
{
	if(b.board[target]!=NULL && b.board[target]->invincible) return false;
	BoardState b_copy = b;
	// Play on empty space and check
	if(b_copy.board[target]==NULL)
	{
		if(b_copy.to_move!=colour) b_copy.play_move(-1);
		if(!b_copy.is_legal_move(target)) return true;
		b_copy.play_move(target);
		return can_capture(b_copy, target, settings.reading_depth, settings, log);
	}
	// Otherwise, correct for colour and then check
	if(b_copy.to_move==b_copy.colour(target)){b_copy.play_move(-1);}
	return can_capture(b_copy, target, settings.reading_depth, settings, log);
}

// Returns true/false. Also outputs which of the two liberties works, if output!=NULL
bool is_ladderable(BoardState& bd, int pos, vector<int>* output, ReadingLog& log)
{
	assert(pos>=0);
	Lump* target = bd.board[pos];
	if(target->invincible) return false;

	if(target->liberties==1) return true;
	if(target->liberties>2) return false;

	BoardState b = bd;
	if(b.to_move==b.colour(pos)) {b.play_move(-1);}
	assert(b.to_move!=b.colour(pos));

	vector<int> libs;
	b.lump_adj_liberties(pos, HV, libs);
	// Put most promising lib first
	int tmp;
	// If one possible move has fewer liberties than the other, play the other one first
	if(b.n_adj_liberties(libs[0], HV)<b.n_adj_liberties(libs[1],HV)){tmp=libs[0];libs[0]=libs[1];libs[1]=tmp;}
	// Also try the first line last
	else if(libs[0]%b.board_size ==0 || libs[0]%b.board_size==b.board_size-1){tmp=libs[0];libs[0]=libs[1];libs[1]=tmp;}
	else if(((int)libs[0]/b.board_size) ==0 || ((int)libs[0]/b.board_size)==b.board_size-1){tmp=libs[0];libs[0]=libs[1];libs[1]=tmp;}

	// Check no of libs for each possible place, play highest 1st
	

	bool lib0legal = b.is_legal_move(libs[0]);
	bool lib1legal = b.is_legal_move(libs[1]);
	if(lib0legal==false && lib1legal==false) {return false;}

	vector<Lump*> adj_friends;
	b.lump_adj_lumps(pos, HV, b.interpret_other_colour(target->colour), adj_friends);

	vector<int> capture_move;
	// Fill this with moves that capture surrounding stones
	for(vector<Lump*>::iterator ii = adj_friends.begin();ii!=adj_friends.end();ii++)
	{
		if((*ii)->liberties==1)
		{
			b.lump_adj_liberties(*ii, HV, capture_move);
		}
	}
	if(capture_move.size()>1) return false;

	//Here we have two moves, not necessarily both legal, and at most one capturing move outside
	bool lib0wins=false;
	bool lib1wins=false;

	//First try lib0 and capturing
	if(lib0legal && (capture_move.size()>0))
	{
		BoardState b_copy = b;
		assert(b_copy.to_move!=b_copy.colour(pos));
		b_copy.play_move(libs[0]);

		if(b_copy.is_legal_move(capture_move[0]))
		{
			b_copy.play_move(capture_move[0]);
			if(is_ladderable(b_copy, pos, NULL, log))
			{
				// This escape fails
			} else {
				// Capture escapes and lib0 fails
				lib0legal=false;
			}
		}
	}




	// Either capturing fails or wasn't possible, so try lib0 and escape by extending
	if(lib0legal)
	{
		BoardState b_copy = b;
		assert(b_copy.to_move!=b_copy.colour(pos));
		b_copy.play_move(libs[0]);

		// If newly-played move is in atari, try capturing it
		if(b_copy.liberties(libs[0])==1)
		{
			vector<int> store;
			b_copy.lump_adj_liberties(libs[0], HV, store);
			if(b_copy.is_legal_move(store[0]))
			{
				BoardState b_copy2 = b_copy;
				b_copy2.play_move(store[0]);
				if(is_ladderable(b_copy2, pos, NULL, log)==false)
				{// Capturing lib0 escapes, and lib0 fails.
					lib0legal=false;
				}
			}
		}

		if(lib0legal)
		{if(b_copy.is_legal_move(libs[1]))
		{
			b_copy.play_move(libs[1]);
			lib0wins = is_ladderable(b_copy, pos, NULL, log);
		} else {
			// Can't play other liberty to escape, so lib0 captures
			lib0wins=true;
		}
		}
	}


	// At this point we've tried lib0. Only try lib1 if we need to.
	if(lib0wins && (output==NULL)) return true;
	// First try lib1 and capture
	if(lib1legal && (capture_move.size()>0))
	{
		BoardState b_copy = b;
		assert(b_copy.to_move!=b_copy.colour(pos));
		b_copy.play_move(libs[1]);

		if(lib1legal && b_copy.is_legal_move(capture_move[0]))
		{
			b_copy.play_move(capture_move[0]);
			if(is_ladderable(b_copy, pos, NULL, log))
			{
				// This escape fails
			} else {
				// Capture escapes and lib0 fails
				lib1legal=false;
			}
		}
	}

	if(lib1legal)
	{
		BoardState b_copy = b;
		assert(b_copy.to_move!=b_copy.colour(pos));
		b_copy.play_move(libs[1]);

		// If newly-played move is in atari, try capturing it
		if(b_copy.liberties(libs[1])==1)
		{
			vector<int> store2;
			b_copy.lump_adj_liberties(libs[1], HV, store2);
			if(b_copy.is_legal_move(store2[0]))
			{
				BoardState b_copy2 = b_copy;
				b_copy2.play_move(store2[0]);
				if(is_ladderable(b_copy2, pos, NULL, log)==false)
				{// Capturing lib1 escapes, and lib1 fails.
					lib1legal=false;
				}
			}
		}
		if(lib1legal)
		{if(b_copy.is_legal_move(libs[0]))
		{
			b_copy.play_move(libs[0]);
			lib1wins = is_ladderable(b_copy, pos, NULL, log);
		} else {
			// Can't play other liberty to escape, so lib0 captures
			lib1wins=true;
		}
		}
	}

	if(output!=NULL)
	{
		if(lib0wins) output->push_back(libs[0]);
		if(lib1wins) output->push_back(libs[1]);
	}
	if(lib0wins==false && lib1wins==false) return false;
	else return true;
}

bool is_ladderable(BoardState& b, Lump& target, vector<int>* output, ReadingLog& log)
{
	return is_ladderable(b, target.stones[0], output, log);
}

// Takes a lump in atari, returns true if it is laddered.
bool is_laddered(BoardState& bd, int pos, ReadingLog& log)
{
	if(bd.liberties(pos)!=1 || bd.board[pos]->invincible) return false;
	
	BoardState b = bd;
	if(b.to_move!=b.colour(pos)) b.play_move(-1);
	
	vector<int> store;
	vector<Lump*> adj_enemies;
	b.lump_adj_lumps(pos, HV, ENEMY, adj_enemies);
	for(vector<Lump*>::iterator ii = adj_enemies.begin(); ii!=adj_enemies.end(); ii++)
	{
		if((*ii)->liberties==1) b.lump_adj_liberties(*ii, HV, store);
	}
	b.lump_adj_liberties(pos, HV, store);
	// Now store contains the one liberty and all adjacent captures.
	// Iterate and see whether any of them escape
	for(vector<int>::iterator ii = store.begin();ii!=store.end();ii++)
	{
		if(b.is_legal_move(*ii))
		{
			BoardState b_copy = b;
			b_copy.play_move(*ii);
			if(is_ladderable(b_copy, pos, NULL, log)==false) return false;
		}
	}
	return true;
}

// Returns true if lump at board position target can be captured after playing
// at most depth moves.
bool can_capture(BoardState& b, int target, int depth, ReadingSettings& settings, ReadingLog& log)
{
	log.addline("Trying to capture in this position");
	log.addboard(b);
	assert(b.board[target]!=NULL);

	// Perform obvious checks
	if(b.board[target]->invincible) return false;
	int target_libs=b.liberties(target);
	if(target_libs==1) {log.addline("...returning true (in atari)"); return true;}
	if((target_libs>depth))
	{
		if(target_libs>2) 
		{
			log.addline("... returning false (too many libs)");
			return false;
		}
	}

	// Check hash table
	int hash_result = b.hash->query_hash(b, true, target, depth);
	if(hash_result==DEAD)
	{
		log.addline("This position is in the hash table - capturable.");
		return true;
	}
	if(hash_result==ALIVE)
	{
		log.addline("This position is in the hash table - not capturable.");
		return false;
	}

	if(is_ladderable(b, target, NULL, log)) 
	{
		log.addline("...returning true (ladderable)"); 
		b.hash->insert_hash(b, true, target, depth, true);
		return true;
	} else if(target_libs==2 && depth<2)
	{
		log.addline("Returning false: too many libs (2, not ladderable)");
		return false;
	}

	// Fill move list and trim it if too large
	vector<int> move_list;
	capture_moves(b, target, depth, move_list, log);
	if(static_cast<int>(move_list.size())>settings.branch_limit)
	{
		move_list.erase(move_list.begin()+settings.branch_limit, move_list.end());
	}

	// For each move on list, create copy, play move, ask about escaping.
	// Also decrement depth

    log.total_moves++;
	for(vector<int>::iterator ii=move_list.begin();ii!=move_list.end();ii++)
	{
		BoardState b_copy = b;
		assert(b_copy.is_legal_move(*ii));
		b_copy.play_move(*ii);

		log.current_depth++;
		bool result = can_escape(b_copy, target, depth-1, settings, log);
		log.current_depth--;

		if(result==false)
		{
			b.hash->insert_hash(b, true, target, depth, true);
			return true;
		}
	}

	// If we tried all capturing moves and none worked, return false
	b.hash->insert_hash(b, true, target, depth, false);
	return false;
}

// Returns false if lump at board position target can be captured when opponent
// plays depth non-atari moves.
bool can_escape(BoardState& b, int target, int depth, ReadingSettings& settings, ReadingLog& log)

{
	log.addline("Trying to escape");
	log.addboard(b);
	assert(b.board[target]!=NULL);

	// Perform obvious checks
	if(b.board[target]->invincible) return true;
	if(b.liberties(target)>=settings.escape_libs)
	{
		return true;
	}

	// Check hash table
	int hash_result = b.hash->query_hash(b, false, target, depth);
	if(hash_result==DEAD)
	{
		log.addline("This position is in the hash table - capturable.");
		return false;
	}
	if(hash_result==ALIVE)
	{
		log.addline("This position is in the hash table - not capturable.");
		return true;
	}

	if(b.liberties(target)==1)
	{
		if(is_laddered(b, target, log)) 
		{
			b.hash->insert_hash(b, false, target, depth, false);
			return false;
		}
	}

	// Fill move list
	vector<int> move_list;
	escape_moves(b, target, depth, move_list, log);
	if(static_cast<int>(move_list.size())>settings.branch_limit)
	{
		move_list.erase(move_list.begin()+settings.branch_limit, move_list.end());
	}

	log.total_moves++;

	// For each move on list, create copy, play move, ask about escaping.
	// If not atari, also decrement depth
	for(vector<int>::iterator ii=move_list.begin();ii!=move_list.end();ii++)
	{
		
		BoardState b_copy = b;
		assert(b_copy.is_legal_move(*ii));
		b_copy.play_move(*ii);
		
		log.current_depth++;
		bool result = can_capture(b_copy, target, depth, settings, log);
		log.current_depth--;

		if(result==false)
		{
			b.hash->insert_hash(b, false, target, depth, true);
			return true;
		}
	}

	// If we tried all escaping moves and none worked, return false
	b.hash->insert_hash(b, false, target, depth, false);
	return false;
}

void capture_moves(BoardState& b, int target, int depth, vector<int>& output, ReadingLog& log)
{
	// We create a list of possible capturing moves of various types:
	// - Liberties of target
	// - Diagonal liberties of target
	// - Defending a surrounding group in atari (either by capturing or extending)

	// Create storage for our moves, to be filled in order and then pushed to output at the end
	vector<int> possible_moves;

	// Liberties of target
	vector<int> target_libs;
	b.lump_adj_liberties(target, HV, target_libs);

	//Check for easy escapes - if a particular escaping move gives you lots of libs then it's the only possible capturing move.
	for(vector<int>::iterator ii = target_libs.begin();ii!=target_libs.end();ii++)
	{
		if(b.resulting_liberties(*ii, b.colour(target))>depth && depth>1)
		{
			if(b.is_legal_move(*ii))
			{
				output.push_back(*ii);
				log.addline("Only one sensible capturing move.");
				return;
			} else {
				log.addline("Only sensible captuirng move is illegal.");
				return;
			}
		}
	}

	// Moves on the diagonal of target that share at least one liberty with target
	vector<int> target_diagonals;
	b.lump_adj_liberties(target, DIAG, target_diagonals);
	for(vector<int>::iterator ii=target_diagonals.begin();ii!=target_diagonals.end();)
	{
		// Erase those diagonals that are already HV liberties
		if(find(target_libs.begin(), target_libs.end(), *ii)!=target_libs.end()) ii=target_diagonals.erase(ii);
		else 
		{
			vector<int> store;
			b.adj_liberties(*ii, HV, store);
			// If this diagonal move shares at least one liberty with target...
			if(vectors_intersect(store, target_libs))
			{
				ii++;
			} else {
				ii=target_diagonals.erase(ii);
			}

		}
	}

	// Moves to defend surrounding groups in atari
	vector<int> defend_atari;
	// List of lumps surrounding target
	vector<Lump*> adj_enemies;
	b.lump_adj_lumps(target, HV, FRIEND, adj_enemies); //FRIEND here because capturer is to move	
	int friendly_in_atari=0;

	for(vector<Lump*>::iterator ii=adj_enemies.begin();ii!=adj_enemies.end();ii++)
	{
		if((*ii)->liberties==1) // If a surrounding group is in atari...
		{
			friendly_in_atari+=(*ii)->size();
			// ...list all groups surrounding *that* in atari, and try capturing them
			vector<Lump*> adj_en_en;
			b.lump_adj_lumps(*ii, HV, ENEMY, adj_en_en);
			for(vector<Lump*>::iterator jj=adj_en_en.begin();jj!=adj_en_en.end();jj++)
			{
				if((*jj)->liberties==1){b.lump_adj_liberties(*jj,HV,defend_atari);}
			}
			// ...and if not caught in a ladder, also try just extending.
			// NB We can't put ladderable check above, because if laddered, we sometimes want to capture anyway, e.g.
			// +------
			// |.OXX..
			// |.XOOOX
			// |..X.X.
			// Also, we want to extend a one-stone laddered lump into two if it might create snapback or damezumari.
			if(!is_laddered(b, (*ii)->stones[0], log)||(b.liberties(target)<=3 &&(*ii)->size()==1)) b.lump_adj_liberties(*ii, HV, defend_atari);
			// Check whether this surrounding stone is essential.
			vector<int> new_libs; // Liberties that would be obtained by target if *ii dies.
			new_libs.clear();
			for(vector<Lump*>::iterator jj=adj_en_en.begin();jj!=adj_en_en.end();jj++)
			{
				b.lump_adj_liberties(*jj, HV, new_libs);
			}
			sort(new_libs.begin(), new_libs.end());
			new_libs.erase(unique(new_libs.begin(), new_libs.end()), new_libs.end());
			if(new_libs.size()>depth+1)
			{
				log.addline("Must capture ");log.addpos((*ii)->stones[0]);
				for(vector<int>::iterator ii=defend_atari.begin();ii!=defend_atari.end();ii++) // Defend atari
				{
					if(find(possible_moves.begin(), possible_moves.end(), *ii)==possible_moves.end()) possible_moves.push_back(*ii);
				}
				log.addline("Found "); log.addint(possible_moves.size());log.add(" capturing moves: ");
				for(vector<int>::iterator ii=possible_moves.begin();ii!=possible_moves.end();ii++)
				{
					log.addpos(*ii);
					if(b.is_legal_move(*ii)) output.push_back(*ii); else log.add("[Illegal]");
				}

			}
		}
	}

	// When capturing, saving surrounding stones gets top priority. They are the very first moves to try.
	for(vector<int>::iterator ii=defend_atari.begin();ii!=defend_atari.end();ii++) // Defend atari
	{
		if(find(possible_moves.begin(), possible_moves.end(), *ii)==possible_moves.end()) possible_moves.push_back(*ii);
	}

	int lib1, lib2;
	int lib1libs, lib2libs;
	bool adjacent;
	// Output high-priority moves here (depending on number of liberties of target)
	switch(b.liberties(target))
	{
	case 1:
		cout<<"CALLED CAPTURE MOVES ON 1 LIB GROUP";
		break;
	case 2:
		// In the case of two liberties, we can choose carefully which one to try first
		sort(target_libs.begin(), target_libs.end());
		lib1 = target_libs[0];
		lib2 = target_libs[1];
		if(b.is_legal_move(lib1)) lib1libs=b.resulting_liberties(lib1, b.to_move);
		if(b.is_legal_move(lib2)) lib2libs=b.resulting_liberties(lib2, b.to_move);
		if(lib2-lib1==1 || lib2-lib1==b.board_size) adjacent=true;
		else adjacent =false;
		
		// If one liberty has only one adjacent liberty, play the other.
		// Also if one liberty has two adjacent spaces, one of which is the other liberty.
		// /*Only if at most one stone in atari*/ Removed "&& friendly_in_atari<=1" from first condition here and below. Seems unnecessary?
		if(b.is_legal_move(lib2)){
			if((lib2libs>1 && b.n_adj_liberties(lib1, HV)==1)
			|| (adjacent && lib2libs>1 && b.n_adj_liberties(lib1, HV)==2))
			{
				if(find(possible_moves.begin(), possible_moves.end(), lib2)==possible_moves.end()) possible_moves.push_back(lib2);
			}
		}
		if(b.is_legal_move(lib1)){
			if((lib1libs>1 && b.n_adj_liberties(lib2, HV)==1)
			|| (adjacent && lib1libs>1 && b.n_adj_liberties(lib2, HV)==2))
			{
				if(find(possible_moves.begin(), possible_moves.end(), lib1)==possible_moves.end()) possible_moves.push_back(lib1);
			}
		}

		// If diagonally adjacent libs and no friendly stones in atari, add net move
		if(((lib2-lib1==b.board_size+1 || lib2-lib1==b.board_size-1)&&(target_diagonals.size()==1))&&friendly_in_atari==0) 
		{
			if((!b.is_atari(lib1, ENEMY))&&(!b.is_atari(lib2, ENEMY)))
			{
			if(find(possible_moves.begin(), possible_moves.end(), lib1+b.board_size)==possible_moves.end()) possible_moves.push_back(lib1+b.board_size);
			if(find(possible_moves.begin(), possible_moves.end(), lib2-b.board_size)==possible_moves.end()) possible_moves.push_back(lib2-b.board_size);
			}
		}

		// Finally, try the space with fewest liberties first
		if(b.is_legal_move(lib2) && b.n_adj_liberties(lib2, HV)>b.n_adj_liberties(lib1, HV))
		{
			if(find(possible_moves.begin(), possible_moves.end(), lib2)==possible_moves.end()) possible_moves.push_back(lib2);
		}

		break;
	case 3:
		break;
	default:
		break;
	}

	// Then output the rest (no duplicates)
	for(vector<int>::iterator ii=target_libs.begin();ii!=target_libs.end();ii++) // Fill liberties
	{
		if(find(possible_moves.begin(), possible_moves.end(), *ii)==possible_moves.end()) possible_moves.push_back(*ii);
	}
	for(vector<int>::iterator ii=target_diagonals.begin();ii!=target_diagonals.end();ii++) // Diagonal moves
	{
		if(find(possible_moves.begin(), possible_moves.end(), *ii)==possible_moves.end()) possible_moves.push_back(*ii);
	}

	// Push the output
	log.addline("Found "); log.addint(possible_moves.size());log.add(" capturing moves: ");
	for(vector<int>::iterator ii=possible_moves.begin();ii!=possible_moves.end();ii++)
	{
		log.addpos(*ii);
		if(b.is_legal_move(*ii)) output.push_back(*ii);
	}
}

void escape_moves(BoardState& b, int target, int depth, std::vector<int>& output, ReadingLog& log)
{
	// Create storage for our moves
	vector<int> possible_moves;

	// Liberties of target
	vector<int> target_libs;
	b.lump_adj_liberties(target, HV, target_libs);
	// Sort them by number of liberties they gain
	struct CompareLibs
	{
		BoardState* b_ptr;
		int colour;
		CompareLibs(BoardState* b_ptr, int colour){this->b_ptr=b_ptr; this->colour=colour;}
		// Order largest first
		bool operator()(int pos1, int pos2)
		{ return b_ptr->resulting_liberties(pos1, colour)> b_ptr->resulting_liberties(pos2, colour);}		
	};

	sort(target_libs.begin(), target_libs.end(), CompareLibs(&b, b.colour(target)));


	// Moves on the diagonal of target that share at least one liberty with target
	vector<int> target_diagonals;
	b.lump_adj_liberties(target, DIAG, target_diagonals);
	for(vector<int>::iterator ii=target_diagonals.begin();ii!=target_diagonals.end();)
	{
		// Remove it if it's already an HV liberty of target
		if(find(target_libs.begin(), target_libs.end(), *ii)!=target_libs.end()) ii=target_diagonals.erase(ii);
		else 
		{
			vector<int> store;
			b.adj_liberties(*ii, HV, store);
			// If this diagonal move shares at least one liberty with target...
			if(vectors_intersect(store, target_libs))
			{
				ii++;
			} else {
				ii=target_diagonals.erase(ii);
			}

		}
	}

	// Moves to capture surrounding groups in atari
	vector<Lump*> adj_enemies;
	b.lump_adj_lumps(target, HV, ENEMY, adj_enemies); //ENEMY here because escaper is to move
	vector<int> capture_outside;
	vector<int> attack_outside;
	vector<int> atari_store;
	for(vector<Lump*>::iterator ii=adj_enemies.begin();ii!=adj_enemies.end();ii++)
	{
		if((*ii)->liberties==1) // If a surrounding group is in atari...
		{
			// ...try capturing it
			b.lump_adj_liberties(*ii, HV, capture_outside);
		}
		if(((*ii)->liberties==2) && (target_libs.size()>1)) // If a surrounding group has two liberties and we have at least two liberties ourselves...
		{
			// ...generate moves to attack it.
			// Put laddering moves in with capturing moves, to try first.
			is_ladderable(b, **ii , &capture_outside, log);
			// Put other ataris into atari store. Any duplicates in this list (double ataris) will get pushed in a minute.
			// The rest go in attack_outside, which has low priority and also goes in time_wasters
			b.lump_adj_liberties(*ii, HV, atari_store);
		}
	}
	// Check atari_store for self-atari and duplicates
	// Duplicates in atari_store are double ataris and should be tried early
	for(vector<int>::iterator kk=atari_store.begin();kk!=atari_store.end();)
	{
		if(b.resulting_liberties(*kk, b.to_move)<=1) kk=atari_store.erase(kk);
		else kk++;
	}
	sort(atari_store.begin(), atari_store.end());
	vector<int>::iterator kk = adjacent_find(atari_store.begin(), atari_store.end());
	while(kk!=atari_store.end())
	{
		// If not self-atari, then push the double atari to possible move list, and delete it.
		if(find(possible_moves.begin(), possible_moves.end(), *kk)==possible_moves.end()
			&& b.resulting_liberties(*kk, b.colour(target))>1) possible_moves.push_back(*kk);
		atari_store.erase(kk, kk+1);
		kk = adjacent_find(atari_store.begin(), atari_store.end());
	}
	
	// The remaining outside ataris do not ladder and aren't double ataris.
	// We only want them if it reduces a surrounding group from three liberties to two
	for(vector<int>::iterator kk = atari_store.begin();kk!=atari_store.end();kk++)
	{
		vector<Lump*> store;
		b.adj_lumps(*kk, HV, ENEMY, store);
		for(vector<Lump*>::iterator ii=adj_enemies.begin();ii!=adj_enemies.end();ii++)
		{
			
			if((*ii)->liberties == 3 && find(store.begin(), store.end(), *ii)!=store.end())
			{
				attack_outside.push_back(*kk);
			}
		}
	}


	// Output any other high-priority moves here
	switch(b.liberties(target))
	{
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	default:
		break;
	}

	// Then output the rest (no duplicates)
	for(vector<int>::iterator ii=capture_outside.begin();ii!=capture_outside.end();ii++) // Capture surrounding groups
	{
		if(find(possible_moves.begin(), possible_moves.end(), *ii)==possible_moves.end()) possible_moves.push_back(*ii);
	}
	for(vector<int>::iterator ii=target_libs.begin();ii!=target_libs.end();ii++) // Extend
	{
		if(find(possible_moves.begin(), possible_moves.end(), *ii)==possible_moves.end()) possible_moves.push_back(*ii);
	}
	for(vector<int>::iterator ii=target_diagonals.begin();ii!=target_diagonals.end();ii++) // Diagonal moves
	{
		if(find(possible_moves.begin(), possible_moves.end(), *ii)==possible_moves.end()) possible_moves.push_back(*ii);
	}
	for(vector<int>::iterator ii=attack_outside.begin();ii!=attack_outside.end();ii++) // Atari surrounding groups
	{ 
		if(find(possible_moves.begin(), possible_moves.end(), *ii)==possible_moves.end()) {possible_moves.push_back(*ii);}
	}

	// Push the output
	log.addline("Found "); log.addint(possible_moves.size());log.add(" escaping moves: ");
	for(vector<int>::iterator ii=possible_moves.begin();ii!=possible_moves.end();ii++)
	{
		log.addpos(*ii);
		if(b.is_legal_move(*ii)) output.push_back(*ii);
	}
}