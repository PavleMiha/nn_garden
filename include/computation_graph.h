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

#define BACKGROUND_IMAGE_RESOLUTION 64
#define NUM_IMAGES 3
class Function {
public:
	char		m_name[128]{ "" };
	unsigned	m_id{ 0 };
	json		m_json;
	int			m_num_inputs;
	int			m_num_outputs;
	bool		m_is_open{ false };
};

// Define a structure to hold the data
struct DataPoint {
	float x;
	float y;
	float label;
};

static const int cross_offsets[5][2] = {
	{ 0,  0 },
	{ -1,  0 },
	{ 0, -1 },
	{ 1,  0 },
	{ 0,  1 } };

static const int x_offsets[5][2] = {
	{ -1, -1 },
	{ -1,  1 },
	{  1, -1 },
	{  1,  1 },
	{  0,  0 } };

class DataSource {
public:

	ImVec2 min = ImVec2(-1, -1);
	ImVec2 max = ImVec2(1, 1);
	int current_data_point = 0;
	std::vector<DataPoint> data;

	DataPoint override_data_point;
	bool should_override_data_point;

	int current_image_index = 0;
	void* image_data[3] = { nullptr };
	void* background_image_data[NUM_IMAGES] = { nullptr, nullptr, nullptr};
	const bgfx::Memory* image_mem[NUM_IMAGES] = { nullptr, nullptr, nullptr};
	const bgfx::Memory* background_image_mem[NUM_IMAGES] = { nullptr, nullptr, nullptr };
	bgfx::TextureHandle image_handle[NUM_IMAGES] = { BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE };
	bgfx::TextureHandle background_image_handle[NUM_IMAGES] = { BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE };

	void set_current_data_point(int index);

	bool load(const char* filename);

	void update_image(ComputationGraph* graph = nullptr);
	void show_body(int attribute_index);
};

class ComputationGraph {
public:
	bool				   used[MAX_NODES];
	Value				   values[MAX_NODES];
	float				   gradient_acc[MAX_NODES];
	Index				   parent[MAX_NODES];
	FunctionNodeData	   function_node_data[MAX_NODES];
	Index				   current_backwards_node = NULL_INDEX;
	Index				   current_result_node = NULL_INDEX;
	Index				   next_free_index = 0;
	int					   current_operation = 0;
	vector<EditOperation>  edit_operations;
	double				   time_right_mouse_pressed = 0.;
	ImVec2				   right_mouse_pressed_pos = ImVec2();
	int					   last_node_hovered = -1;
	ImNodesMiniMapLocation minimap_location;
	DataSource			   data_source;
	vector<Index>		   nodes_to_select;

	std::map<Index, std::vector<Index>> cached_topological_sorted_descendants;
	unordered_set <Index> cached_no_parent;
	vector<Index> cached_sorted_descendants;

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

	void randomize_parameters();
	
	void do_stochastic_gradient_descent_step(float learning_rate);
	void do_stochastic_gradient_descent(float learning_rate, int batch_size, int& current_point, const vector<int> shuffled_points);

	void forwards(float* data_values);
	void backwards(float* data_values);

	void initialise() {
		data_source.load("flat.csv");
		data_source.set_current_data_point(0);
	}
	void zero_gradients();

	Value& get_value(Index index);

	struct FromJsonAdditionalData {
		vector<Index> indices;
		vector<Socket> unmatched_inputs;
		vector<Socket> unmatched_outputs;
	};

	vector<Index> from_json(const json& json, const ImVec2& origin, FromJsonAdditionalData* additional_data = nullptr);

	json to_json(Index* indices, const ImVec2& origin, int num) const;

	json to_json() const;

	void add_operation_without_applying(EditOperation& operation);

	void apply_operation(EditOperation& operation);

	void undo();

	void redo();

	void copy_selected_nodes(const ImVec2& origin);

	void paste(const ImVec2& origin);

	void delete_selected_nodes();

	unsigned get_attribute_input_index(Index i, unsigned input);

	unsigned get_attribute_output_index(Index i, unsigned output);

	void render_gradient(Index i, float node_width);

	void show_node(Index i, vector<Function>& functions);

	void show_connection(Connection connection);

	void show_connections(Index i, vector<Function>& functions);

	void create_connection(Connection connection);

	void show(const int editor_id, bool* open, std::vector<Function>& functions, const char* name);

	bool selected = false;

};