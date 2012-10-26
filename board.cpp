//Contains the board data structure and methods for reading and updating it. It also contains the lump class
//
#include <algorithm>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <assert.h>
#include "board.h"
#include "globals.h"


using namespace std;


//A lump is a string of solidly-connected stones on the board. Lumps (rather than individual stones) are the basic tactical unit,
//so the lumps are tracked by the board state and updated as each move is played. The list of board positions is stored as an STL vector.




//Create a new lump of one stone, colour c, position pos
Lump::Lump(int pos, int c)
{
	stones.push_back(pos);
	index=pos;
	colour=c;
	liberties=4;
	invincible=false;
}

string Lump::display(void)
{
	stringstream ss;
	ss << "Lump "<<size()<<" stones:";
	if(invincible) ss<<" (invincible) ";
	for(vector<int>::iterator ii=stones.begin();ii!=stones.end();ii++)
	{
		ss<<" ("<<(*ii)%DEBUG_BS<<","<<(int)(*ii/DEBUG_BS)<<")";
	}
	ss<<(colour==BLACK ? "\nBlack, " : "\nWhite, ");
	ss<<liberties<<" liberties";

	return ss.str();	
}

//The board comprises an array of pointers to objects of type "lump", the integer board_size, a to_move marker, and a ko_point marker.
//Everything else (captured stones, komi, time, move history) handled by gamestate object. This is the object used for reading.

BoardState::BoardState(int b)
{
	board_size=b;
	board.resize(board_size*board_size, NULL);
	to_move=BLACK;
	ko_marker=-1;
	hash_value=0;
	hash=NULL;
}

BoardState::BoardState(const BoardState& other)
{
	board_size=other.board_size;
	to_move=other.to_move;
	ko_marker=other.ko_marker;
	hash=other.hash;
	hash_value=other.hash_value;

	map<Lump*, Lump*> already_done;
	map<Lump*,Lump*>::iterator jj;
	for(vector<Lump*>::const_iterator ii=other.board.begin();ii!=other.board.end();ii++)
	{
		if(*ii==NULL)
		{
			board.push_back(NULL);
			continue;
		}
		if((jj=already_done.find(*ii))==already_done.end())
		{
			Lump* new_lump = new Lump(**ii);
			already_done.insert(pair<Lump*,Lump*>(*ii, new_lump));
			board.push_back(new_lump);
		} else {
			board.push_back(jj->second);
		}
	}
}
//Destructor must delete all Lump objects
BoardState::~BoardState(void)
{
	vector<Lump*>::iterator ii;
	for(ii=board.begin();ii!=board.end();ii++)
	{
		if(*ii!=NULL)
		{
			delete_lump(*ii);
		}
	}
}

//Print the board state to the terminal
void BoardState::display(void)
{
	cout << "\n\n  ";
	for(int i=0;i<board_size;i++){cout << i%10;}
	cout<<"\n +";
	for(int i=0;i<board_size;i++){cout << "-";}
	cout << "+  "<<(to_move==BLACK ? "Black" : "White") <<" to move\n";
	for(int j=0;j<board_size;j++)
	{
		cout << j%10 << "|";
		for(int i=0;i<board_size;i++)
		{
			if(board[(board_size*j)+i]==NULL)
			{
				cout<<(ko_marker==(board_size*j)+i ? ":" : ".");
			} else {
				if(board[(board_size*j)+i]->colour==BLACK)
				{
					cout<<(board[(board_size*j)+i]->invincible ? "k" : "X");
				} else {
					cout<<(board[(board_size*j)+i]->invincible ? "c" : "O");
				}
			}
		}
		cout<<"|";
		if(j==1) cout<<"  Hash "<<hash_value;
		cout<<"\n";
	}
	cout <<" +";
	for(int i=0;i<board_size;i++){cout << "-";}
	cout << "+\n";
}	

//Returns true if pos is a legal move for the current colour
bool BoardState::is_legal_move(int pos)
{
	if(pos==-1) return true;
	if(board[pos]!=NULL || pos==ko_marker){return false;} //Must be empty, not ko
	if(n_adj_liberties(pos, HV)>0){return true;} //Ok if has liberties. If not...

	vector<Lump*> adj_enemies;
	adj_lumps(pos, HV, ENEMY, adj_enemies);
	for(vector<Lump*>::iterator ii=adj_enemies.begin();ii!=adj_enemies.end();ii++)
	{
		if((*ii)->liberties==1 && (!(*ii)->invincible)){return true;} //Return true if neighbour in atari and not invincible
	}
	vector<Lump*> adj_friends;
	adj_lumps(pos, HV, FRIEND, adj_friends);
	for(vector<Lump*>::iterator ii=adj_friends.begin();ii!=adj_friends.end();ii++)
	{
		if((*ii)->liberties>1 || (*ii)->invincible){return true;} //Return true if friend has more than one lib or invincible
	}

	return false;
}

//To delete a lump, set all board positions listed in stones vector to NULL, then delete.
//Return number of stones in the lump.
int BoardState::delete_lump(Lump* target)
{
	assert(target!=NULL);
	vector<int>::iterator ii;
	for(ii=target->stones.begin();ii!=target->stones.end();ii++)
	{
		board[*ii]=NULL; //set board entry to NULL
		hash->remove_stone(*ii, target->colour, hash_value); // Update hash
	}
	int size=target->size();
	delete target;
	return size;
}

//Merge lump2 into lump1. Delete lump2, update board
void BoardState::merge_two_lumps(Lump& lump1, Lump& lump2)
{
	lump1.stones.insert(lump1.stones.end(), lump2.stones.begin(), lump2.stones.end());
	lump1.index = min(lump1.index, lump2.index);
	for(vector<int>::iterator ii=lump2.stones.begin(); ii!=lump2.stones.end(); ii++)
	{
		board[*ii]=&lump1;
	}
	if(lump1.invincible||lump2.invincible) lump1.invincible=true;
	delete &lump2;
}

int BoardState::recalculate_liberties(Lump& target)
{
	vector<int> liberty_list;
	for(vector<int>::iterator ii=target.stones.begin();ii!=target.stones.end();ii++)
	{
		adj_liberties(*ii,HV,liberty_list);
	}
	sort(liberty_list.begin(), liberty_list.end());
	liberty_list.erase(unique(liberty_list.begin(),liberty_list.end()),liberty_list.end());
	target.liberties = liberty_list.size();
	return liberty_list.size();
}
// Play a move at given position. Perform relevant captures, merge lumps, update ko position,
// increment to_move. Return number of captured stones (return negative number for suicide).
// Asserts move is legal move, but checking should be done elsewhere.
// Also updates hash value in place by calling hash.add/remove_stone
void BoardState::play_move(int pos)
{
	assert(is_legal_move(pos));
	ko_marker=-1;
	// Deal with pass separately
	if(pos==-1)
	{
		to_move=(to_move==BLACK ? WHITE : BLACK);
		return;
	} else {
		// Otherwise, have to update hash
		hash->add_stone(pos, to_move, hash_value);
	}

	board[pos]= new Lump(pos, to_move);
	vector<Lump*> adj_friends;
	vector<Lump*> adj_enemies;
	adj_lumps(pos, HV, FRIEND, adj_friends);
	adj_lumps(pos, HV, ENEMY, adj_enemies);
	// Merge
	for(vector<Lump*>::iterator ii = adj_friends.begin();ii!=adj_friends.end();ii++)
	{
		merge_two_lumps(*board[pos], **ii);
	}
	// Delete captured stones
	int captured=0;
	vector<Lump*> store;
	for(vector<Lump*>::iterator ii = adj_enemies.begin();ii!=adj_enemies.end();ii++)
	{
		if((*ii)->liberties==1 && !(*ii)->invincible)
		{
			// Store lumps adjacent to captured lump, to update liberties later
			lump_adj_lumps(*ii, HV, FRIEND, store);
			captured+=delete_lump(*ii);
		}
	}
	// Detect ko
	if(captured==1 && adj_friends.size()==0 && n_adj_liberties(pos, HV)==1)
	{
		vector<int> libs;
		adj_liberties(pos, HV, libs);
		ko_marker=libs[0];
	}

	// Recalculate liberties of relevant lumps
	adj_lumps(pos, HV, ENEMY, store);
	store.push_back(board[pos]);
	for(vector<Lump*>::iterator ii = store.begin();ii!=store.end();ii++)
	{
		recalculate_liberties(**ii);
	}	
	// (If you change this bit, don't forget to change the pass behaviour above)
	to_move=(to_move==BLACK ? WHITE : BLACK);
}

//Returns BLACK, WHITE or interprets FRIEND, ENEMY relative to board.to_move
int BoardState::interpret_colour(int colour)
{
	if (colour==BLACK 
	|| (colour==FRIEND && to_move==BLACK) 
	|| (colour==ENEMY && to_move==WHITE))
	{return BLACK;} else {return WHITE;}
}
int BoardState::interpret_other_colour(int colour)
{
	if (colour==BLACK 
	|| (colour==FRIEND && to_move==BLACK) 
	|| (colour==ENEMY && to_move==WHITE))
	{return WHITE;} else {return BLACK;}
}

// Push to output a list of all points in a taxicab radius of r from pos
void BoardState::taxicab_radius(int pos, int r, std::vector<int>& output)
{
	int i, j;
	i=pos%board_size;
	j=(int)pos/board_size;
	for(int k=-r;k<=r;k++)
	{ for(int l=-r;l<=r;l++)
	{
		if((abs(k)+abs(l)<=r) && i+k>=0 && i+k<board_size && j+l>=0 && j+l<board_size)
		{
			output.push_back(((j+l)*board_size)+i+k);
		}
	}}
}
// Push to output a list of all lumps in a taxicab radius of r from pos
void BoardState::taxicab_radius_lumps(int pos, int r, std::vector<Lump*>& output)
{
	vector<int> store;
	vector<Lump*> store2;
	taxicab_radius(pos, r, store);
	for(vector<int>::iterator ii=store.begin();ii!=store.end();ii++)
	{
		if(board[*ii]!=NULL) store2.push_back(board[*ii]);
	}
	sort(store2.begin(), store2.end());
	store2.erase(unique(store2.begin(), store2.end()),store2.end());
	output.insert(output.end(), store2.begin(), store2.end());
	
}

//Push all adjacent board positions into output vector. Choose horizontal/vertical, diagonal,
//or both with directions= HV, DIAG, HVDIAG. Output is board positions (integers)
void BoardState::adj_points(int pos, int directions, vector<int>& output)
{
	assert(pos>=0);
	int on_edge=0; //bit 1 for top edge, bit 2 for bottom edge, 4 for left, 8 for right
	if(pos<board_size){on_edge+=1;}
	if(pos>=board_size*(board_size-1)){on_edge+=2;}
	if(pos%board_size==0){on_edge+=4;}
	if(pos%board_size==board_size-1){on_edge+=8;}

	if((directions&HV)!=0) // if reporting horizontal/vertical locations
	{
		if((on_edge&1)==0){output.push_back(pos-board_size);}
		if((on_edge&2)==0){output.push_back(pos+board_size);}
		if((on_edge&4)==0){output.push_back(pos-1);}
		if((on_edge&8)==0){output.push_back(pos+1);}
	}

	if((directions&DIAG)!=0) // if reporting diagonal locations
	{
		if((on_edge&5)==0){output.push_back(pos-board_size-1);}
		if((on_edge&9)==0){output.push_back(pos-board_size+1);}
		if((on_edge&6)==0){output.push_back(pos+board_size-1);}
		if((on_edge&10)==0){output.push_back(pos+board_size+1);}
	}
}

void BoardState::adj_liberties(int pos, int directions, vector<int>& output)
{
	assert(pos>=0);
	vector<int> store;
	adj_points(pos, directions, store);
	for(vector<int>::iterator ii=store.begin();ii!=store.end();ii++)
	{
		if(board[*ii]==NULL){output.push_back(*ii);}
	}
}

int BoardState::n_adj_liberties(int pos, int directions)
{
	assert(pos>=0);
	vector<int> store;
	adj_liberties(pos, directions, store);
	return store.size();
}

void BoardState::lump_adj_points(int pos, int directions, std::vector<int>& output)
{
	assert(pos>=0);
	lump_adj_points(board[pos], directions, output);
}
void BoardState::lump_adj_points(Lump* target, int directions, std::vector<int>& output)
{
	vector<int> store;
	for(vector<int>::iterator ii=target->stones.begin();ii!=target->stones.end();ii++)
	{
		adj_points(*ii, directions, store);
	}
	sort(store.begin(), store.end());
	store.erase(unique(store.begin(), store.end()),store.end());
	for(vector<int>::iterator ii = store.begin();ii!=store.end();ii++){output.push_back(*ii);}
}

void BoardState::lump_adj_liberties(int pos, int directions, std::vector<int>& output)
{
	assert(pos>=0);
	lump_adj_liberties(board[pos], directions, output);
}
void BoardState::lump_adj_liberties(Lump* target, int directions, std::vector<int>& output)
{
	assert(target!=NULL);
	vector<int> store;
	for(vector<int>::iterator ii=target->stones.begin();ii!=target->stones.end();ii++)
	{
		adj_liberties(*ii, directions, store);
	}
	sort(store.begin(), store.end());
	store.erase(unique(store.begin(), store.end()),store.end());
	for(vector<int>::iterator ii = store.begin();ii!=store.end();ii++){output.push_back(*ii);}
}

int BoardState::lump_n_adj_liberties(int pos, int directions)
{
	assert(pos>=0);
	return lump_n_adj_liberties(board[pos], directions);
}
int BoardState::lump_n_adj_liberties(Lump* target, int directions)
{
	assert(target!=NULL);
	vector<int> store;
	lump_adj_liberties(target, directions, store);
	return store.size();
}

// Return the number of liberties a move at pos would end up with, including captures.
// Return 1000 if it connects to an invincible group.
// If the position is already the given colour, just return the number of liberties
int BoardState::resulting_liberties(int pos, int colour)
{
	assert(pos!=-1);
	assert(board[pos]==NULL);
	// Adjacent friends and enemies
	vector<Lump*> adj_f;
	adj_lumps(pos, HV, colour, adj_f);
	vector<Lump*> adj_e;
	adj_lumps(pos, HV, interpret_other_colour(colour), adj_e);
	// Now make a list of all points adjacent to the resulting group
	vector<int> adj_pts;
	adj_points(pos, HV, adj_pts);
	for(vector<Lump*>::iterator ii = adj_f.begin();ii!=adj_f.end();++ii)
	{
		if((*ii)->invincible) return 1000;
		lump_adj_points(*ii, HV, adj_pts);
	}
	sort(adj_pts.begin(), adj_pts.end());
	adj_pts.erase(unique(adj_pts.begin(), adj_pts.end()), adj_pts.end());
	// Create a list filled with pointers that will indicate a to-be-empty space on the board
	vector<Lump*> will_be_empty;
	will_be_empty.push_back(NULL);
	for(vector<Lump*>::iterator ii = adj_e.begin(); ii!=adj_e.end(); ++ii)
	{
		if(lump_n_adj_liberties(*ii, HV)==1 && !(*ii)->invincible) will_be_empty.push_back(*ii);
	}
	int result = 0;
	for(vector<int>::iterator jj = adj_pts.begin();jj!=adj_pts.end();++jj)
	{
		if((*jj)==pos) continue;
		if(find(will_be_empty.begin(), will_be_empty.end(), board[*jj])!=will_be_empty.end()) result++;
	}
	return result;
}

// Returns true if a move at pos by colour puts anything in atari
bool BoardState::is_atari(int pos,int colour)
{
	vector<Lump*> store;
	adj_lumps(pos, HV, interpret_other_colour(colour), store);
	for(vector<Lump*>::iterator ii=store.begin();ii!=store.end();ii++)
	{if((*ii)->liberties==2 && (!(*ii)->invincible)) return true;}
	return false;
}

// Returns true if a move at pos by colour captures anything
bool BoardState::is_capture(int pos,int colour)
{
	vector<Lump*> store;
	adj_lumps(pos, HV, interpret_other_colour(colour), store);
	for(vector<Lump*>::iterator ii=store.begin();ii!=store.end();ii++)
	{if((*ii)->liberties==1 && (!(*ii)->invincible)) return true;}
	return false;
}
//Push onto output all the lumps on the board.
void BoardState::all_lumps(vector<Lump*>& output)
{
	vector<Lump*> list_of_lumps;
	for(int i=0;i<board_size*board_size; i++)
	{
		if(board[i]!=NULL){list_of_lumps.push_back(board[i]);}
	}
	sort(list_of_lumps.begin(), list_of_lumps.end());
	list_of_lumps.erase(unique(list_of_lumps.begin(), list_of_lumps.end()), list_of_lumps.end());
	for(vector<Lump*>::iterator ii=list_of_lumps.begin(); ii!=list_of_lumps.end(); ii++)
	{
		output.push_back(*ii);
	}
}

// Push onto output a list of pointers to lumps adjacent to pos, of given colour. 
// Removes duplicates
void BoardState::adj_lumps(int pos, int directions, int colour, vector<Lump*>& output)
{
	assert(pos>=0);
	colour=interpret_colour(colour);
	vector<int> points;
	vector<Lump*> lumps;
	adj_points(pos, directions, points); //get list of board points adjacent.
	for(vector<int>::iterator ii=points.begin();ii!=points.end();ii++)
	{//fill lumps with pointers to lump objects from board
		if(board[*ii]!=NULL){lumps.push_back(board[*ii]);}
	} 
	//sort, remove duplicates, erase useless end of the vector
	sort(lumps.begin(), lumps.end());
	lumps.erase(unique(lumps.begin(), lumps.end()), lumps.end() );

	//Take resulting list of lumps, return the ones we want
	for(vector<Lump*>::iterator ii=lumps.begin();ii!=lumps.end();ii++)
	{
		if((*ii)->colour==colour)
		{
			output.push_back(*ii); 
		}
	}
}

// Push onto output a list of pointers to lumps adjacent to pos, of given colour and with only one liberty
// Removes duplicates
void BoardState::adj_lumps_captured(int pos, int directions, int colour, vector<Lump*>& output)
{
	assert(pos>=0);
	colour=interpret_colour(colour);
	vector<int> points;
	vector<Lump*> lumps;
	adj_points(pos, directions, points); //get list of board points adjacent.
	for(vector<int>::iterator ii=points.begin();ii!=points.end();ii++)
	{//fill lumps with pointers to lump objects from board
		if(board[*ii]!=NULL){lumps.push_back(board[*ii]);}
	} 
	//sort, remove duplicates, erase useless end of the vector
	sort(lumps.begin(), lumps.end());
	lumps.erase(unique(lumps.begin(), lumps.end()), lumps.end() );

	//Take resulting list of lumps, return the ones we want
	for(vector<Lump*>::iterator ii=lumps.begin();ii!=lumps.end();ii++)
	{
		if((*ii)->colour==colour && (*ii)->liberties==1)
		{
			output.push_back(*ii); 
		}
	}
}

// Push onto output a list of lumps adjacent to the given one.
// Removes duplicates
void BoardState::lump_adj_lumps(int pos, int directions, int colour, vector<Lump*>& output)
{
	assert(pos>=0);
	lump_adj_lumps(board[pos], directions, colour, output);
}
void BoardState::lump_adj_lumps(Lump* target, int directions, int colour, vector<Lump*>& output)
{
	vector<Lump*> store;
	for(vector<int>::iterator ii=target->stones.begin();ii!=target->stones.end();ii++)
	{
		adj_lumps(*ii, directions, colour, store);
	}
	sort(store.begin(), store.end());
	store.erase(unique(store.begin(), store.end()),store.end());
	for(vector<Lump*>::iterator ii = store.begin();ii!=store.end();ii++){output.push_back(*ii);}
}

// Pushes to output a list of integer board positions adjacent to pos of given colour
// that are not empty. Removes duplicates.
void BoardState::adj_lump_pos(int pos, int directions, int colour, vector<int>& output)
{
	assert(pos>=0);
	colour=interpret_colour(colour);
	vector<int> points;
	adj_points(pos, directions, points); //get list of board points adjacent.
	for(vector<int>::iterator ii=points.begin();ii!=points.end();ii++)
	{
		if(board[*ii]!=NULL)
		{
			if(board[*ii]->colour==colour){output.push_back(*ii);}
		}
	}
}

// Returns true if pos is adjacent to at least one stone of given colour
bool BoardState::is_adj_to(int pos, int directions, int colour)
{
	assert(pos>=0);
	colour=interpret_colour(colour);
	vector<int> points;
	adj_points(pos, directions, points); //get list of board points adjacent.
	for(vector<int>::iterator ii=points.begin();ii!=points.end();ii++)
	{
		if(board[*ii]!=NULL && board[*ii]->colour==colour) return true;
	}
	return false;
}


// Returns true if target lump has any liberties inside the given box (and not in extra_border).
// (Used in alive_in_box L&D reading)
bool BoardState::has_libs_in_box(int target, int left, int right, int top, int bottom, std::vector<int>& extra_border)
{
	vector<int> store;
	lump_adj_liberties(target, HV, store);
	for(vector<int>::iterator ii=store.begin();ii!=store.end();ii++)
	{
		int x = (*ii)%board_size;
		int y = (int)(*ii)/board_size;
		if(x>left && x<right && y>top && y<bottom && find(extra_border.begin(), extra_border.end(), *ii)==extra_border.end()) return true;
	}
	return false;
}