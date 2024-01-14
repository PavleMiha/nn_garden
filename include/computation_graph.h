#pragma once

#include "imnodes.h"
#include "value.h"
#include "edit_operation.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <unordered_set>
#include <iostream>

#include <bgfx/bgfx.h>
#include "stb/stb_image.h"
#include "clip.h"
#include "json.hpp"

#include "bigg.hpp"

#define MAX_NODES 1024*1024
#define MAX_CONNECTIONS_PER_NODE 64

class Function {
public:
	char		m_name[128]{ "" };
	unsigned	m_id{ 0 };
	json		m_json;
	int			m_num_inputs;
	int			m_num_outputs;
	bool		m_is_open{ false };
};

class DataSource {
public:
	void*				data;
	bgfx::TextureHandle image_handle = BGFX_INVALID_HANDLE;

	void set_current_image_index(int index);

	void load();
	void show();
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
	ImNodesMiniMapLocation minimap_location;
	DataSource			   data_source;

	void clear();

	ComputationGraph() {
		printf("Computation Graph constructor\n");

		clear();
	}

	void collapse_selected_nodes_to_new_function(const ImVec2& origin, vector<Function>& functions);

	void collapse_to_new_function(Index* indices, size_t num_indices, ImVec2 pos, vector<Function>& functions);

	Index collapse_to_function(Index* indices, size_t num_indices, ImVec2 pos, int function_id);

	Index instantiate_function(int function_id, const ImVec2& pos, const vector<Function>& functions);

	Index get_new_value();

	void delete_value_and_return_removed_connections(Index index, vector<Connection>& removed_connections);

	void forwards();

	void initialise() {
		data_source.load();
		data_source.set_current_image_index(0);
	}
	void zero_gradients();

	Value& get_value(Index index);

	struct FromJsonAdditionalData {
		vector<Index> indices;
		vector<Connection> unmatched_inputs;
		vector<Connection> unmatched_outputs;
	};

	void from_json(const json& json, const ImVec2& origin, FromJsonAdditionalData* additional_data = nullptr);

	json to_json(Index* indices, const ImVec2& origin, int num);

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

	void show_node(Index i, vector<Function>& functions);

	void show_connection(Connection connection);

	void show_connections(Index i, vector<Function>& functions);

	void create_connection(Connection connection);

	void show(const int editor_id, bool* open, std::vector<Function>& functions, const char* name);

	bool selected = false;

};