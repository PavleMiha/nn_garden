#pragma once

#include "computation_graph.h"

class Context {
public:
	Index					   current_backwards_node = NULL_INDEX;
	ComputationGraph		   main_graph;
	float					   learning_rate = 0.01f;
	int						   training_steps = 0;
	float					   current_average_error = 0;

	map<int, ComputationGraph> function_graphs;
	std::vector<Function>	   functions;

	bool					   m_training = false;	

	Context() {
	}

	void clear();

	int create_function(const json& json_data);

	void show_training_menu(bool* open);

	void show_function_list(bool* open);

	void create_function_graph(int function_index);

	void show(bool* open);

	void save(const char* filename);

	void load(const char* filename);
};
