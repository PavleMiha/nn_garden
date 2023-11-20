#pragma once

#include "imnodes.h"
#include "value.h"
#include "edit_operation.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <unordered_set>
#include <iostream>
#include "clip.h"
#include "json.hpp"

#define MAX_NODES 1024*1024
#define MAX_CONNECTIONS_PER_NODE 64

class Function {
public:
	char		m_name[128]{ "" };
	unsigned	m_id{ 0 };
	json		m_json;
	bool		m_is_open{ false };
};

class ComputationGraph {
public:
	bool				   used[MAX_NODES];
	Value				   values[MAX_NODES];
	Index				   parent[MAX_NODES];
	FunctionNodeData	   function_node_data[MAX_NODES];
	Index				   current_backwards_node = NULL_INDEX;
	Index				   next_free_index = 0;
	int					   current_operation = 0;
	vector<EditOperation>  edit_operations;
	double				   time_right_mouse_pressed = 0.;
	int					   last_node_hovered = -1;
	ImNodesMiniMapLocation minimap_location_;

	void clear();

	ComputationGraph() {
		printf("Computation Graph constructor\n");
		clear();
	}

	void collapse_selected_nodes_to_function(const ImVec2& origin, vector<Function>& functions);

	void collapse_to_function(int* indices, size_t num_indices, ImVec2 pos, vector<Function>& functions);

	Index get_new_value();

	void delete_value_and_return_removed_connections(Index index, vector<Connection>& removed_connections);

	void forwards();

	void zero_gradients();

	Value& get_value(Index index);

	void from_json(const json& json, const ImVec2& origin);

	json to_json(int* indices, const ImVec2& origin, int num);

	void add_operation_without_applying(EditOperation& operation);

	void apply_operation(EditOperation& operation);

	void undo();

	void redo();

	void copy_selected_nodes(const ImVec2& origin);

	void paste(const ImVec2& origin);

	void delete_selected_nodes();

	void save(const char* filename);

	void load(const char* filename);

	unsigned get_attribute_input_index(Index i, unsigned input);

	unsigned get_attribute_output_index(Index i, unsigned output);

	void show(const int editor_id, bool* open, std::vector<Function>& functions, const char* name);

	bool selected = false;

};