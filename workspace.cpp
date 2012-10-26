#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include "globals.h"
#include <time.h>
#include "board.h"
#include "hashing.h"
#include "reading.h"
#include "lifedeath.h"
#include "TLGETC.h" 
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

using namespace std;

// Class containing window dimensions, board measurements, and data for drawing debugging output
class WindowData
{
public:
	int width;
	int height;
	int margin;
	int board_size;
	int square_width;
	int half_width;

	map<int, sf::Color> team_colours;

	WindowData(int w, int h, int m, int bs) : height(h), width(w), margin(m), board_size(bs)
	{
		square_width = (int)(height - (2*margin))/board_size;
		if(square_width%2==0) square_width++;
		half_width = (square_width-1)/2;
	}

	int get_i(int x){return get_i((float)x);}
	int get_j(int y){return get_j((float)y);}
	int get_i(float x) // Get i coordinate from window x coordinate (return -1 if not on board)
	{
		int result = (int)(x-margin)/square_width;
		if(result<0 || result>=board_size) return -1;
		return result;
	}
	int get_j(float y) // Get j coordinate from window y coordinate (return -1 if not on board)
	{
		int result = (int)(y-margin)/square_width;
		if(result<0 || result>=board_size) return -1;
		return result;
	}
	float get_x(int i)
	{
		return (float)margin + half_width + (i*square_width);
	}
	float get_y(int j)
	{
		return (float)margin + half_width + (j*square_width);
	}

	void draw_circle(int i, int j, int x_offset, int y_offset, int radius, sf::Color colour, sf::RenderWindow& window)
	{
		sf::CircleShape circle((float)radius);
		circle.setFillColor(colour);
		circle.setPosition(get_x(i)-radius + x_offset, get_y(j)-radius + y_offset);
		window.draw(circle);
	}
};


// Draw board (don't display)
void draw_board(sf::RenderWindow& window, WindowData& wd, BoardState& b, Engine& engine)
{
	sf::RectangleShape board_rect;
	board_rect.setSize(sf::Vector2f((float)wd.height, (float)wd.height));
	board_rect.setFillColor(sf::Color::Color(193, 154, 107));
	board_rect.setPosition(0, 0);
	window.draw(board_rect);

	sf::VertexArray lines(sf::Lines, 0);
	for(int i=0;i<b.board_size;i++)
	{
		lines.append(sf::Vertex(sf::Vector2f(wd.get_x(0), wd.get_y(i)), sf::Color::Black));
		lines.append(sf::Vertex(sf::Vector2f(wd.get_x(wd.board_size-1), wd.get_y(i)), sf::Color::Black));
		lines.append(sf::Vertex(sf::Vector2f(wd.get_x(i), wd.get_y(0)), sf::Color::Black));
		lines.append(sf::Vertex(sf::Vector2f(wd.get_x(i), wd.get_y(wd.board_size-1)), sf::Color::Black));
	}
	window.draw(lines);

	// Draw on stones and ko marker
	for(int i=0;i<b.board_size*b.board_size;i++)
	{
		if(b.board[i]!=NULL)
		{
			wd.draw_circle(i%wd.board_size,(int)i/wd.board_size,0,0, wd.half_width - 1, (b.board[i]->colour==BLACK?sf::Color::Black : sf::Color::White), window);
		}
		if(i==b.ko_marker)
		{
			wd.draw_circle(i%wd.board_size,(int)i/wd.board_size,0,0, (int)wd.half_width/5, sf::Color::Color(128, 128, 128), window);
		}
	}

	// Draw on team markers and interesting-lump markers.
	Engine* ep = &engine;
	TLGETC* p = static_cast<TLGETC*>(ep);
	for(vector<Team*>::iterator ii = p->teams.begin();ii!=p->teams.end();ii++)
	{
		sf::Color team_colour;
		map<int, sf::Color>::iterator tc;
		tc = wd.team_colours.find((*ii)->lumps[0]->stones[0]);
		if(tc==wd.team_colours.end())
		{
			team_colour = sf::Color::Color(rand()%255, rand()%255, rand()%255);
			wd.team_colours[(*ii)->lumps[0]->stones[0]] = team_colour;
		} else {
			team_colour=wd.team_colours[(*ii)->lumps[0]->stones[0]];
		}		 

		for(vector<Lump*>::iterator jj = (*ii)->lumps.begin();jj!=(*ii)->lumps.end();jj++)
		{
		for(vector<int>::iterator kk = (*jj)->stones.begin();kk!=(*jj)->stones.end();kk++)
		{
			wd.draw_circle((*kk)%wd.board_size,(int)(*kk)/wd.board_size,0,0, (int)wd.half_width/4, team_colour, window);
			if(find(p->interesting_lumps.begin(), p->interesting_lumps.end(), *jj)!=p->interesting_lumps.end())
			{
				wd.draw_circle((*kk)%wd.board_size,(int)(*kk)/wd.board_size,-10,-10, 3, sf::Color::Color(128,128,128), window);
			}
		}
		}
	}



}

void parse_command(string input, BoardState& board, Engine& engine)
{
	stringstream ss(input);
	string command;
	ss>>command;
	if(command=="move" || command=="mvoe" || command=="m")
	{
		int x, y;
		ss>>x;
		ss>>y;
		if(board.is_legal_move(x+board.board_size*y))
		{
			cout<<"\nPlaying move at ("<<x<<","<<y<<")\n";
			engine.make_move(x+board.board_size*y);
		} else {
			cout<<"\nIllegal move";
		}
	} else if(command=="?")
	{
		int chosen_move = engine.get_move();
		cout<<"Engine selects ("<<chosen_move%board.board_size<<","<<(int)chosen_move/board.board_size<<")\n";
	} else if(command=="info"||command=="i")
	{
		int x, y;
		ss>>x;
		ss>>y;
		if(board.board[x+board.board_size*y]==NULL)
		{
			cout<<"\nEmpty point.";
		} else {
			cout<<board.board[(x+board.board_size*y)]->display();
			cout<<"\nEngine data:\n";
			Engine* p_engine = &engine;
			TLGETC* p_TLGETC = static_cast<TLGETC*>(p_engine);
			cout<<p_TLGETC->lump_data[p_TLGETC->b.board[x+board.board_size*y]].display(); // This line is revolting
		}
	} else if(command=="hash"||command=="i")
	{
		Engine* p_engine = &engine;
		TLGETC* p_TLGETC = static_cast<TLGETC*>(p_engine);
		cout<<"\nThe hash table currently has "<<p_TLGETC->hash.hash_table.size()<<" entries.\n";
		cout<<"It has served "<<p_TLGETC->hash.queries<<" queries, of which "<<p_TLGETC->hash.positive_queries<<" were successful.\n";
		cout<<(((_int64)1)<<(33+9));
	} else if (command=="lumps" || command=="l")
	{
		vector<Lump*> list_of_lumps;
		board.all_lumps(list_of_lumps);
		for(vector<Lump*>::iterator ii=list_of_lumps.begin();ii!=list_of_lumps.end();ii++)
		{
			cout<<"\n"<<(*ii)->display();
		}
	} else if (command=="teams" || command=="t")
	{
		Engine* p_engine = &engine;
		TLGETC* p_TLGETC = static_cast<TLGETC*>(p_engine);
		cout<<"\n"<<p_TLGETC->teams.size()<<" teams:\n";
		for(vector<Team*>::iterator ii=p_TLGETC->teams.begin();ii!=p_TLGETC->teams.end();ii++)
		{
			cout<<"Team with lumps ";
			for(vector<Lump*>::iterator jj = (*ii)->lumps.begin(); jj!=(*ii)->lumps.end();jj++)
			{
				cout<<"("<<(*jj)->stones[0] % board.board_size<<","<<(int)(*jj)->stones[0] / board.board_size<<") ";
			}
			cout<<"\n";
		}
	} else if (command=="ladderable" || command=="il" || command=="lad")
	{
		int x, y;
		ss>>x;
		ss>>y;
		if(board.board[x+board.board_size*y]==NULL)
		{
			cout<<"\nEmpty point.";
		} else {
			ReadingLog log;
			int pos = (x+board.board_size*y);
			vector<int> results;
			bool wins = is_ladderable(board, pos, &results, log);
			if(wins)
			{
				cout<<"\nLadderable at ";
				for(vector<int>::iterator ii=results.begin();ii!=results.end();ii++)
				{
					cout<<"("<<(*ii)%board.board_size<<" "<<(int) (*ii)/board.board_size<<") ";
				}
			} else {
				cout<<"\nNot ladderable.";
			}
		}
	} else if (command=="capture" || command=="cap")
	{
		int x, y;
		ss>>x;
		ss>>y;
		if(board.board[x+board.board_size*y]==NULL)
		{
			cout<<"\nEmpty point.";
		} else {
			ReadingLog log;
			int pos = (x+board.board_size*y);
			vector<int> capture_here;
			vector<int> escape_here;
			ReadingSettings settings;
			int result = get_status(board, pos, true, &capture_here, &escape_here, settings, log);

			if(result==ALIVE)
			{
				if(pos==49) cout<<"Asking about 49: ";
				cout<<"\nNo capturing moves found.";
			} else {
				cout<<"\nCapturing moves at ";
				for(vector<int>::iterator ii=capture_here.begin();ii!=capture_here.end();ii++)
				{
					cout<<"("<<(*ii)%board.board_size<<" "<<(int) (*ii)/board.board_size<<") ";
				}
				if(result==UNSETTLED){
					cout<<"\nEscaping moves at ";
					for(vector<int>::iterator ii=escape_here.begin();ii!=escape_here.end();ii++)
					{
						cout<<"("<<(*ii)%board.board_size<<" "<<(int) (*ii)/board.board_size<<") ";
					}
				} else{
					cout<<"\nNo escaping moves found.";
				}
			}
			cout<<"\nTotal moves read: "<<log.total_moves;
			cout<<"\nTime taken: " << difftime(time(NULL),log.start_time);
			
		}
	} else if(command=="pass" || command=="p") {
		engine.make_move(-1);
	} else if(command=="box" || command=="iba") {
		int l, r, t, b, target;
		ss>>l>>r>>t>>b>>target;
		vector<int> targets;
		targets.push_back(target);
		vector<int> extra_border;
		ReadingLog log;
		log.addline("starting...");
		get_status_in_box(board,targets, l, r, t, b, extra_border, log); 
	} else if(command=="ipa")
	{		
		int x, y;
		ss>>x;
		ss>>y;
		vector<int> empty_vector;
		ReadingLog log;
		if(is_pass_alive(board, x+board.board_size*y, empty_vector, log)) cout<<"\nPass-alive.";
		else cout<<"\nNot pass-alive.";
	} else {
		cout<<"\nWhat?";
	}
}


int main(int argc, char* argv[])
{
	cout << "Testing The Little Go Engine That Could v0.1\n";

	TLGETC engine(9);
	engine.b.display();
	string command;

	WindowData wd(800,600,50,engine.board_size);

	sf::RenderWindow window(sf::VideoMode(wd.width, wd.height), "The Little Go Engine That Could");
	window.setFramerateLimit(60);
	window.display();

	// Begin window redraw-refresh loop
	while(window.isOpen())
	{
		// See if there are any events to be handled
		sf::Event the_event;
		while (window.pollEvent(the_event))
		{
		  // Request for closing the window
		   if (the_event.type == sf::Event::Closed)
		       return 0;
		   // If a button was pressed, read from terminal until we quit
		   if(the_event.type==sf::Event::KeyPressed)
		   {
			   while(true)
			   {
					cout<<"Enter command: ";
					getline(cin, command);
					if(command=="quit" || command=="q") break;
					parse_command(command, engine.b, engine);
					engine.b.display();
			   }
			   while(window.pollEvent(the_event)){} // There must be a method to clear the event queue?
		   }
		   // If click, try to play move there
		   if(the_event.type==sf::Event::MouseButtonPressed)
		   {
			   sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
			   int x = mouse_pos.x;
			   int y = mouse_pos.y;
			   int i = wd.get_i(x);
			   int j = wd.get_j(y);
			   if(i!=-1 && j!=-1)
			   {
				   stringstream ss;
				   ss<<"move "<<i<<" "<<j;
				   parse_command(ss.str(),engine.b, engine);
			   }
		   }
		}	
		window.clear();
		// Draw the board
		draw_board(window, wd, engine.b, engine);
		window.display();
	}
	return 0;
}

