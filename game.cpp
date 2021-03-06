#include "game.h"
#include "node.h"
#include "io.h"
#include <iostream>
#include <chrono>
#include <cmath>


namespace {

	const int COMPUTER_WIN = 5000;
	const int PLAYER_WIN = -5000;
	const int MAX_DEPTH = 25;
	const char TERMINATE = '\0';

}


Game::Game() {

}


Game::~Game() {
	
}


void Game::next_move(Node* node) {

	int best_score = 0;
	int best_element = -1;
	int depth = 1;

	this->init_clock();

	while (depth < MAX_DEPTH) {
		int element = this->minimax(node, depth);
		int score = this->calculate_config_score(node, SYMBOL::PLAYER);
		if (score > best_score) {
			best_score = score;
			best_element = element;
		}
		node->config_[element] = SYMBOL::EMPTY;
		++depth;
	}

	node->config_[best_element] = SYMBOL::PLAYER;
}


int Game::minimax(Node* node, int depth) {
	
	int best = INT_MIN;
	int score = 0;
	int alpha = INT_MIN;
	int beta = INT_MAX;
	int element = -1;

	std::vector<int> moves = this->query_possible_moves(node);
	for (size_t i = 0; i < moves.size(); ++i) {
		node->config_[moves[i]] = SYMBOL::PLAYER;
		score = this->minimize(node, alpha, beta, depth - 1);
		if (score > best) {
			best = score;
			element = moves[i];
		}
		node->config_[moves[i]] = SYMBOL::EMPTY;
	}
	node->config_[element] = SYMBOL::PLAYER;
	return element;
}


int Game::minimize(Node* node, int& alpha, int& beta, int depth) {

	int best = INT_MAX;
	int score;

	if (this->won_game(node, SYMBOL::PLAYER))
		return
			this->calculate_config_score(node, SYMBOL::PLAYER);
	if (this->won_game(node, SYMBOL::OPPONENT))
		return
			-this->calculate_config_score(node, SYMBOL::OPPONENT);
	if (depth == 0 || this->time_expired())
		return 
			this->calculate_config_score(node, SYMBOL::PLAYER) - 
			this->calculate_config_score(node,SYMBOL::OPPONENT);

	bool unpruned = true;
	std::vector<int> moves = this->query_possible_moves(node);
	for (size_t i = 0; i < moves.size() && unpruned; ++i) {

		node->config_[moves[i]] = SYMBOL::OPPONENT;
		score = this->maximize(node, alpha, beta, depth - 1);
		if (score < best)
			best = score;
		node->config_[moves[i]] = SYMBOL::EMPTY;

		alpha = alpha > score ? alpha : score;
		if (alpha > beta)
			unpruned = false;

	}
	return best;
}


int Game::maximize(Node* node, int& alpha, int& beta, int depth) {

	int best = INT_MIN;
	int score;

	if (this->won_game(node, SYMBOL::PLAYER))
		return
			this->calculate_config_score(node, SYMBOL::PLAYER);
	if (this->won_game(node, SYMBOL::OPPONENT))
		return
			-this->calculate_config_score(node, SYMBOL::OPPONENT);
	if (depth == 0 || this->time_expired())
		return
			this->calculate_config_score(node, SYMBOL::PLAYER) -
			this->calculate_config_score(node, SYMBOL::OPPONENT);

	bool unpruned = true;
	std::vector<int> moves = this->query_possible_moves(node);
	for (size_t i = 0; i < moves.size() && unpruned; ++i) {

		node->config_[moves[i]] = SYMBOL::PLAYER;
		score = this->minimize(node, alpha, beta, depth - 1);
		if (score > best)
			best = score;
		node->config_[moves[i]] = SYMBOL::EMPTY;

		beta = beta < score ? beta : score;
		if (alpha > beta)
			unpruned = false;

	}
	return best;
}


int Game::calculate_config_score(Node* node, char player) {

	int utility = 0;

	for (int r = 0; r < dim::SPAN; ++r) {
		for (int c = 0; c < dim::SPAN; ++c) {

			int index = r*dim::SPAN + c;
			char symbol = node->config_[index];

			if (symbol == player) {

				char left_symbols[dim::MAX_ADJ];
				char right_symbols[dim::MAX_ADJ];
				char up_symbols[dim::MAX_ADJ];
				char down_symbols[dim::MAX_ADJ];

				for (int i = 0; i < dim::MAX_ADJ; ++i) {
					left_symbols[i] = right_symbols[i] = 
						up_symbols[i] = down_symbols[i] = '\0';
				}

				for (int i = 1; i < dim::MAX_ADJ; ++i) {
					if (c - i >= 0) {
						char left_sym = node->config_[index-i];
						left_symbols[i-1] = left_sym;
					}
					if (c + i < dim::SPAN) {
						char right_sym = node->config_[index+i];
						right_symbols[i-1] = right_sym;
					}
				}

				for (int i = 1; i < dim::MAX_ADJ; ++i) {
					if (r - i >= 0) {
						char up_sym = node->config_[index-i*dim::SPAN];
						up_symbols[i-1] = up_sym;
					}
					if (r + i < dim::SPAN) {
						char down_sym = node->config_[index+i*dim::SPAN];
						down_symbols[i-1] = down_sym;
					}
				}
				
				utility += this->utility_stream(player,
					left_symbols, right_symbols, 
					up_symbols, down_symbols);

			}
		}
	}

	return utility;

}


int Game::utility_stream(char player,
	char left[], char right[],
	char up[], char down[])
{
	int utility = 0;

	utility += this->utility_player_axis(player,left,right);
	utility += this->utility_player_axis(player,up,down);

	return utility;
}


int Game::utility_player_axis(char player, char dir_a[], char dir_b[]) {

	int utility = 0;
	int consec_plyr_a = 0, consec_plyr_b = 0;
	int a = 0, b = 0;
	int empty = 0;
	int consec_oppn_a = 0, consec_oppn_b = 0;
	
	this->utility_array_player(player, dir_a, consec_plyr_a, a, empty);
	this->utility_array_player(player, dir_b, consec_plyr_b, b, empty);

	this->utility_opponent_axis(player, dir_a, consec_oppn_a);
	this->utility_opponent_axis(player, dir_b, consec_oppn_b);

	if (empty >= dim::MAX_ADJ) {
		utility += (consec_plyr_a+consec_plyr_b);
	}
	utility += (consec_oppn_a + consec_oppn_b);
	utility += empty;

	char sym_a = dir_a[0];
	char sym_b = dir_b[0];

	if (sym_a == sym_b && sym_a != SYMBOL::EMPTY && sym_a != '\0') {
		utility += dim::MAX_ADJ * 2;
	}

	return utility;

}


int Game::utility_array_player(char player, char dir[],
	int& plyr, int& oppn, int& empty)
{
	int utility = 0;
	bool uninterrupted = true;
	bool consecutive = true;
	for (int i = 0; i < dim::MAX_ADJ - 1; ++i) {
		if (player == dir[i]) {
			if (uninterrupted) {
				plyr += dim::MAX_ADJ - i;
				++empty;
			}
			if (consecutive) {
				plyr += ((i + 1)*dim::MAX_ADJ);
			}
		}
		else if (dir[i] == SYMBOL::EMPTY) {
			if (uninterrupted) {
				++empty;
			}
			consecutive = false;
		}
		else {
			uninterrupted = consecutive = false;
		}
	}
	
	return utility;

}


int Game::utility_opponent_axis(char player, char dir[], int& oppn) {

	int utility = 0;
	bool consecutive = true;

	char opponent = player == SYMBOL::PLAYER ? 
		SYMBOL::OPPONENT : SYMBOL::PLAYER;

	for (int i = 0; i < dim::MAX_ADJ - 1; ++i) {
		if (dir[i] == opponent) {
			if (consecutive) {
				oppn += ((i + 3)*(dim::MAX_ADJ));
			}
		}
		else {
			consecutive = false;
		}

	}

	return utility;

}


int Game::specific_utility(char player, char dir[]) {

	int utility = 0;
	char opponent = player == SYMBOL::PLAYER ? 
		SYMBOL::OPPONENT : SYMBOL::PLAYER;
	if (dir[0] == opponent) {
		utility += 5;
	}


	return utility;

}


int Game::utility_array(char player, char symbol,
	int& consec_player, bool& uninter_player, int& consec_oppn, 
	int& consec_spaces, bool& uninter_spaces) 
{
	if (symbol == player) {
		if (uninter_player) {
			++consec_player;
		}
	}
	else if (symbol == SYMBOL::EMPTY) {
		if (uninter_spaces) {
			++consec_spaces;
		}
	}
	else {
		if (symbol != '\0') {
			++consec_oppn;
		}
		uninter_player = uninter_spaces = false;
	}
	return 0;
}



int Game::compass_utility(int plyr_count, int oppn_count, int open_spaces) {
	int utility = 0;	
	if (plyr_count + open_spaces >= dim::MAX_ADJ-1)
		utility += this->pow(plyr_count, plyr_count);
	if (oppn_count > 1)
		utility += this->pow(oppn_count, oppn_count);
	return utility;
}


int Game::axis_utility(
	int plyr_left, int plyr_right,
	int oppn_left, int oppn_right,
	int spaces_left, int spaces_right)
{
	int utility = 0;
	if ((plyr_left + plyr_right + spaces_left + spaces_right) >= dim::MAX_ADJ) {
		utility += this->pow(plyr_left + plyr_right, plyr_left+plyr_right);
	}
	if (oppn_left + oppn_right > 1) {
		utility += this->pow(oppn_left + oppn_right, oppn_left + oppn_right);
	}
	return utility;
}


void Game::create_compass(char symbols[], int index, char curr_sym, char player_sym) {

	if (curr_sym == player_sym) {

	}
	else if (curr_sym == SYMBOL::EMPTY) {

	}
	else {

	}


}


void Game::compare_counter(
	char curr_sym, char player_sym,
	bool& free_player, int& player_count,
	bool& is_open, int& open_spaces,
	bool& free_oppn, int& oppn_count)
{
	if (curr_sym == player_sym) {
		if (free_player) {
			++player_count;
		}
	}
	else if (curr_sym == SYMBOL::EMPTY) {
		if (is_open) {
			++open_spaces;
		}
	}
	else {
		++oppn_count;
		free_player = is_open = false;
	}
}



// span is column or row, iter is +1 for up or -1 for down
// offset refers to +1/-1 for cols, +8/-8 for rows
char* Game::survey_direction(Node* node, int span, int iter, int offset) {
	int init = span;
	//char symbols[dim::MAX_ADJ-1];

	char* symbols = new char[dim::MAX_ADJ - 1];

	for (int i = 0; i < dim::MAX_ADJ - 1; ++i) {
		span += iter;
		char symbol = node->config_[(init+i)*offset];
		if (span < 0 || span > dim::SPAN - 1) {
			symbols[i] = TERMINATE;
		}
		else {
			symbols[i] = symbol;
		}
	}
	return symbols;
}



int Game::calculate_config_rows(Node* node, char player) {
	int row_score = 0;
	for (int row = 0; row < dim::SPAN; ++row) {
		int r = row*dim::SPAN;
		for (int c = 0; c <= dim::MAX_ADJ; ++c) {
			int uninterrupted = 0, consec_sym = 0, consec_counter = 0;
			for (int i = 0; i < dim::MAX_ADJ && uninterrupted != -1; ++i) {
				char symbol = node->config_[r+c+i];
				if (symbol == player) {
					++uninterrupted;
					++consec_sym;
					if (consec_sym > 1)
						++consec_counter;
					if (uninterrupted > 2)
						++consec_counter;
				}
				else if (symbol == SYMBOL::EMPTY) {
					consec_sym = 0;
				}
				else {
					uninterrupted = -1;
				}
			}
			if (uninterrupted > 0)
				row_score += this->pow(uninterrupted, consec_counter);
		}
	}
	return row_score;
}


int Game::calcualte_config_cols(Node* node, char player) {
	int column_score = 0;
	for (int c = 0; c < dim::SPAN; ++c) {
		for (int r = 0; r <= dim::MAX_ADJ; ++r) {
			int uninterrupted = 0, consec_sym = 0, consec_counter = 0;
			for (int i = 0; i < dim::MAX_ADJ && uninterrupted != -1; ++i) {
				char symbol = node->config_[c+dim::SPAN*(r+i)];
				if (symbol == player) {
					++uninterrupted;
					++consec_sym;
					if (consec_sym > 1)
						++consec_counter;
					if (uninterrupted > 2)
						++consec_counter;
				}
				else if (symbol == SYMBOL::EMPTY) {
					consec_sym = 0;
				}
				else {
					uninterrupted = -1;
				}
			}
			if (uninterrupted > 0)
				column_score += this->pow(uninterrupted, consec_counter);
		}
	}
	return column_score;
}


int Game::utility_offensive(Node* node) {
	return
		this->calculate_config_score(node, SYMBOL::PLAYER) -
		this->calculate_config_score(node, SYMBOL::OPPONENT);
}


int Game::utility_defensive(Node* node) {
	return 
		(-this->calculate_config_score(node,SYMBOL::OPPONENT));
}


std::vector<int> Game::query_possible_moves(Node* node) {
	std::vector<int> moves;
	for (int i = 0; i < dim::SIZE; ++i)
		if (node->config_[i] == SYMBOL::EMPTY)
			moves.push_back(i);
	return moves;
}


void Game::insert_unique(std::vector<int>& vec, int elem) {
	for (size_t i = 0; i < vec.size(); ++i)
		if (vec[i] == elem)
			return;
	vec.push_back(elem);
}


// this works
int Game::update_min_max_row(Node* node, int row, char player) {
	int row_score = 0;
	int r = row*dim::SPAN;
	for (int c = 0; c <= dim::MAX_ADJ; ++c) {
		int uninterrupted = 0;
		for (int i = 0; i < dim::MAX_ADJ && uninterrupted != -1; ++i) {
			char symbol = node->config_[r+c+i];
			if (symbol == player)
				++uninterrupted;
			else if (symbol != SYMBOL::EMPTY)
				uninterrupted = -1;
		}
		row_score += (uninterrupted != -1 ? uninterrupted : 0);
	}
	return row_score;
}


// this works
int Game::update_min_max_col(Node* node, int col, char player) {
	int c = col;
	int column_score = 0;
	for (int r = 0; r <= dim::MAX_ADJ; ++r) {
		int uninterrupted = 0;
		for (int i = 0; i < dim::MAX_ADJ && uninterrupted != -1; ++i) {
			char symbol = node->config_[c+dim::SPAN*(r+i)];
			if (symbol == player)
				++uninterrupted;
			else if (symbol != SYMBOL::EMPTY)
				uninterrupted = -1;
		}
		column_score += (uninterrupted != -1 ? uninterrupted : 0);
	}
	return column_score;
}


void Game::make_first_move(Node* node) {
	int row = 3, col = 3;
	node->config_[row*dim::SPAN+col] = SYMBOL::PLAYER;
}


// should be used when placing symbols using the row and col numbers
// that are indeces in the array
void Game::place_symbol_by_indeces(Node* node, char symbol, int row, int col) {
	node->config_[row*dim::SPAN+col] = symbol;
}


bool Game::get_offensive_strategy() {
	return this->offensive_strategy_;
}


void Game::set_max_depth(int depth) {
	
}


void Game::set_time_allowed(int time_allowed) {
	this->time_allowed_ = time_allowed*1000000000;
}


void Game::set_strategy_offensive() {
	this->offensive_strategy_ = true;
}


void Game::set_strategy_defensive() {
	this->offensive_strategy_ = false;
}


void Game::revise_strategy(Node* node) {
	
	int player_score = this->calculate_config_score(node, SYMBOL::PLAYER);
	int opponent_score = this->calculate_config_score(node, SYMBOL::OPPONENT);
	this->offensive_strategy_ = player_score >= opponent_score;

}


void Game::init_clock() {
	algorithm_time_ = 
		std::chrono::high_resolution_clock::now().time_since_epoch().count();
}


bool Game::time_expired() {
	long long range = 
		std::chrono::high_resolution_clock::now().time_since_epoch().count() - 
		algorithm_time_;
	return range >= this->time_allowed_;
}


void Game::clear_nodes(std::vector<Node*>& nodes) {
	for (size_t i = 0; i < nodes.size(); ++i) {
		if (nodes[i] != nullptr) {
			delete nodes[i];
			nodes[i] = nullptr;
		}
	}
}


// this works
bool Game::won_game(Node* node, char symbol) {

	for (int r = 0; r < dim::SPAN; ++r) {
		int consecutive = 0;
		for (int c = 0; c < dim::SPAN; ++c) {
			if (node->config_[r*dim::SPAN+c] == symbol) {
				++consecutive;
				if (consecutive == dim::MAX_ADJ) {
					return true;
				}
			}
			else {
				consecutive = 0;
			}
		}
	}

	for (int c = 0; c < dim::SPAN; ++c) {
		int consecutive = 0;
		for (int r = 0; r < dim::SPAN; ++r) {
			if (node->config_[r*dim::SPAN + c] == symbol) {
				++consecutive;
				if (consecutive == dim::MAX_ADJ) {
					return true;
				}
			}
			else {
				consecutive = 0;
			}
		}
	}

	return false;
}


bool Game::tied_game(Node* node) {
	for (int i = 0; i < dim::SIZE; ++i) {
		if (node->config_[i] == SYMBOL::EMPTY) {
			return false;
		}
	}
	return true;
}


int Game::pow(int base, int exp) {
	int p = 1;
	for (int i = 0; i < exp; ++i)
		p *= base;
	return p;
}


