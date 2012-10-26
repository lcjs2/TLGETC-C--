#ifndef LIFEDEATH_H_080812
#define LIFEDEATH_H_080812

#include <vector>
#include "reading.h"


// EyeGraph contains data about the eyespace in a box.
class EyeGraph
{
public:
	std::vector<int> spaces; // Empty spaces where eyes can be made
	std::vector<int> adj_spaces; // Empty spaces adjacent to enemy stones

	std::vector<std::vector<int> > all_enemy_stones; // Enemy stones in the box that are not invincible
	std::vector<std::vector<int> > dead_stones; // Captured enemy stones
	std::vector<std::vector<int> > killable_stones; // Capturable enemy stones

	std::vector<int> vital_points;
	//  Points in eyespace with a certain number of neighbours
	std::vector<int> zero_nb;
	std::vector<int> one_nb;
	std::vector<int> two_nb;
	std::vector<int> three_nb;
	std::vector<int> four_nb;

	std::string display();
};

bool is_pass_alive(BoardState& b, int pos, std::vector<int>& codependants, ReadingLog& log);
bool is_epa(BoardState& b, int pos, ReadingSettings& settings, ReadingLog& log);
int get_status_in_box(BoardState& b, std::vector<int> targets, int left, int right, int top, int bottom, std::vector<int>& extra_border, ReadingLog& log);
void box_compute_eyespace(BoardState&b, int left, int right, int top, int bottom, int colour, EyeGraph& otuput, ReadingLog& log);
int eval_eyespace_I(EyeGraph& eyespace, int board_size, std::vector<int>& output);

#endif