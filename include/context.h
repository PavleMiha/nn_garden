#pragma once

#include "computation_graph.h"

class Context {
public:
	Index					   current_backwards_node = NULL_INDEX;
	ComputationGraph		   main_graph;
	map<int, ComputationGraph> function_graphs;
	std::vector<Function>	   functions;

	Context() {
		printf("Context constructor\n");
		//clear();
	}

	void clear();

	int create_function(const json& json_data);

	void show_function_list(bool* open);

	void create_function_graph(int function_index);

	void show(bool* open);

	void save(const char* filename);

	void load(const char* filename);
};
