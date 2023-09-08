#include "imnodes.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <unordered_set>
#include <iostream>
#include "clip.h"
#include "json.hpp"

#define MAX_NODES 1024*1024
#define MAX_CONNECTIONS_PER_NODE 64
#define NULL_INDEX UINT_MAX
using std::vector;
using std::map;
using std::unordered_set;
using nlohmann::json;

enum class Operation {
	Add,
	Subtract,
	Multiply,
	Divide,
	Power,
	Function,
	None,
};

typedef unsigned Index;

struct Connection {
public:
	Connection(Index _index, unsigned short _input_slot, unsigned short _output_slot) :
		index(_index), input_slot(_input_slot), output_slot(_output_slot) {};
	Connection() : index(NULL_INDEX), input_slot(0), output_slot(0) {};
	Index index { NULL_INDEX };
	unsigned short input_slot{ 0 };
	unsigned short output_slot{ 0 };
};

class FunctionNodeData {
public:
	short				   m_function_id;
	vector<Connection>	   m_function_input_nodes;
	vector<Connection>	   m_function_output_nodes;

};
#define NUM_INPUTS 2

class Value {
public:
	Index			   m_index{ NULL_INDEX };
	float			   m_value{ 0.f };
	float			   m_gradient{ 0.f };
	Operation		   m_operation{ Operation::None };
	Connection		   m_inputs[NUM_INPUTS];
	Index			   m_parent{ NULL_INDEX };

	json to_json() {
		json j;
		j["index"]	   = m_index;
		j["value"]	   = m_value;
		j["gradient"]  = m_gradient;
		j["operation"] = m_operation;
		for (int i = 0; i < NUM_INPUTS; i++) {
			if (m_inputs[i].index != NULL_INDEX) {
				j["inputs"][i]["index"] = m_inputs[i].index;
				j["inputs"][i]["input_slot"] = m_inputs[i].input_slot;
				j["inputs"][i]["output_slot"] = m_inputs[i].output_slot;
			}
		}
		return j;
	}

	void from_json(json j) {
		m_index	   = j["index"];
		m_value	   = j["value"];
		m_gradient = j["gradient"];
		m_operation = j["operation"];
		for (int i = 0; i < j["inputs"].size(); i++) {
			int slot = j["inputs"][i]["input_slot"];
			m_inputs[slot].index	   = j["inputs"][i]["index"];
			m_inputs[slot].input_slot  = j["inputs"][i]["input_slot"];
			m_inputs[slot].output_slot = j["inputs"][i]["output_slot"];
		}
	}

	Value() {};
	Value(float value) : m_value(value) {};
	Value(float value, Operation operation, Index parent1, Index parent2) :
		m_value(value),
		m_operation(operation),
		m_inputs()
		{};

	void set_operation(Operation operation) {
		m_operation = operation;
	}

	vector<Index> get_topological_sorted_descendants(Value* values) {
		unordered_set<Index> visited;
		return get_topological_sorted_descendants_inner(visited, values);
	}

	vector<Index> get_topological_sorted_descendants_inner(unordered_set<Index>& visited, Value* values) {
		vector<Index> sorted_descendants;
		visited.insert(m_index);
		for (const auto& input : m_inputs) {
			if (input.index != NULL_INDEX) {
				vector<Index> m_parent1_descendants = values[input.index].get_topological_sorted_descendants_inner(visited, values);
				sorted_descendants.insert(sorted_descendants.end(), m_parent1_descendants.begin(), m_parent1_descendants.end());
			}
		}
		sorted_descendants.push_back(m_index);
		return sorted_descendants;
	}

	void single_forwards(Value* values) {
		switch (m_operation) {
		case Operation::Add:
			if (m_inputs[0].index != NULL_INDEX && m_inputs[1].index != NULL_INDEX)
				m_value = values[m_inputs[0].index].m_value + values[m_inputs[1].index].m_value;
			break;
		case Operation::Multiply:
			if (m_inputs[0].index != NULL_INDEX && m_inputs[1].index != NULL_INDEX)
				m_value = values[m_inputs[0].index].m_value * values[m_inputs[1].index].m_value;
			break;
		case Operation::Subtract:
			if ( m_inputs[0].index != NULL_INDEX && m_inputs[1].index != NULL_INDEX)
				m_value = values[m_inputs[0].index].m_value - values[m_inputs[1].index].m_value;
			break;
		case Operation::Divide:
			if (m_inputs[0].index != NULL_INDEX && m_inputs[1].index != NULL_INDEX)
				m_value = values[m_inputs[0].index].m_value / values[m_inputs[1].index].m_value;
			break;
		case Operation::Power:
			if (m_inputs[0].index != NULL_INDEX && m_inputs[1].index != NULL_INDEX)
				m_value = pow(values[m_inputs[0].index].m_value, values[m_inputs[1].index].m_value);
			break;
		default:
			//assert((false && "Unknown operation"));
			break;
		}
	}

	void single_backwards(Value* values) {
		switch (m_operation) {
		case Operation::Add:
			if (m_inputs[0].index != NULL_INDEX && m_inputs[1].index != NULL_INDEX) {
				values[m_inputs[0].index].m_gradient += m_gradient;
				values[m_inputs[1].index].m_gradient += m_gradient;
			}
			break;
		case Operation::Multiply:
			if (m_inputs[0].index != NULL_INDEX && m_inputs[1].index != NULL_INDEX) {
				values[m_inputs[0].index].m_gradient += m_gradient * values[m_inputs[1].index].m_value;
				values[m_inputs[1].index].m_gradient += m_gradient * values[m_inputs[0].index].m_value;
			}
			break;
		case Operation::Subtract:
			if (m_inputs[0].index != NULL_INDEX && m_inputs[1].index != NULL_INDEX) {
				values[m_inputs[0].index].m_gradient += m_gradient;
				values[m_inputs[1].index].m_gradient -= m_gradient;
			}
			break;
		case Operation::Divide:
			if (m_inputs[0].index != NULL_INDEX && m_inputs[1].index != NULL_INDEX) {
				values[m_inputs[0].index].m_gradient += m_gradient / values[m_inputs[1].index].m_value;
				values[m_inputs[1].index].m_gradient -= m_gradient * values[m_inputs[0].index].m_value / (values[m_inputs[1].index].m_value * values[m_inputs[1].index].m_value);
			}
			break;
		case Operation::Power:
			if (m_inputs[0].index != NULL_INDEX && m_inputs[1].index != NULL_INDEX) {
				values[m_inputs[0].index].m_gradient += m_gradient * values[m_inputs[1].index].m_value * pow(values[m_inputs[0].index].m_value, values[m_inputs[1].index].m_value - 1);
				values[m_inputs[1].index].m_gradient += m_gradient * pow(values[m_inputs[0].index].m_value, values[m_inputs[1].index].m_value) * log(values[m_inputs[0].index].m_value);
			}
			break;
		default:
			//assert((false && "Unknown operation"));
			break;
		}
	}

	void backwards(Value* values) {
		vector<Index> sorted_descendants = get_topological_sorted_descendants(values);

		for (auto& it : sorted_descendants) {
			values[it].m_gradient = 0.f;
		}

		m_gradient = 1.f;

		for (vector<Index>::reverse_iterator it = sorted_descendants.rbegin(); it != sorted_descendants.rend(); ++it) {
			values[*it].single_backwards(values);
		}
	}

	static Value make_value() {
		Value value;
		value.m_operation = Operation::None;
		return value;
	}

	static Value make_add() {
		Value value;
		value.m_operation = Operation::Add;
		return value;
	}

	static Value make_multiply() {
		Value value;
		value.m_operation = Operation::Multiply;
		return value;
	}

	static Value make_subtract() {
		Value value;
		value.m_operation = Operation::Subtract;
		return value;
	}

	static Value make_divide() {
		Value value;
		value.m_operation = Operation::Divide;
		return value;
	}

	static Value make_power() {
		Value value;
		value.m_operation = Operation::Power;
		return value;
	}

};

namespace ImGui
{
	bool SelectableInput(const char* str_id, bool selected, ImGuiSelectableFlags flags, char* buf, size_t buf_size)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImVec2 pos_before = window->DC.CursorPos;

		PushID(str_id);
		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(g.Style.ItemSpacing.x, g.Style.FramePadding.y * 2.0f));
		bool ret = Selectable("##Selectable", selected, flags | ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowItemOverlap);
		PopStyleVar();

		ImGuiID id = window->GetID("##Input");
		bool temp_input_is_active = TempInputIsActive(id);
		bool temp_input_start = ret ? IsMouseDoubleClicked(0) : false;

		if (temp_input_start)
			SetActiveID(id, window);

		if (temp_input_is_active || temp_input_start)
		{
			ImVec2 pos_after = window->DC.CursorPos;
			window->DC.CursorPos = pos_before;
			ret = TempInputText(g.LastItemData.Rect, id, "##Input", buf, (int)buf_size, ImGuiInputTextFlags_AutoSelectAll);
			window->DC.CursorPos = pos_after;
		}
		else
		{
			window->DrawList->AddText(pos_before, GetColorU32(ImGuiCol_Text), buf);
		}

		PopID();
		return ret;
	}
}

class ComputationGraph;
class Context;

enum class EditOperationType {
	AddNode,
	RemoveNode,
	AddLink,
	RemoveLink,
	MoveNodes,
	SetBackwardsNode
};

class EditOperation {
public:
	EditOperationType m_type;
	Value			  m_value;
	Index			  m_index;
	Index			  m_previousIndex;
	Connection		  m_connection;
	ImVec2			  m_position;
	ImVec2			  m_pos_delta;
	bool			  m_final;

	void EditOperation::apply(ComputationGraph* context);
	void EditOperation::undo(ComputationGraph* context);
	static EditOperation add_node(const Value& value, const ImVec2& pos, const bool _final = true);
	static EditOperation remove_node(const Index index, const bool _final = true);
	static EditOperation add_link(const Connection& connection, const Index index, const bool _final = true);
	static EditOperation remove_link(const Connection& connection, const Index index, const bool _final = true);
	static EditOperation move_node(const Index index, const ImVec2& delta, const bool _final = true);
};

class Function {
public:
	char		m_name[128]{ "" };
	unsigned	m_id{ 0 };
	json		m_json;
	bool		m_is_open{ false };
};

class ComputationGraph {
public:
	bool				   used				 [MAX_NODES];
	Value				   values			 [MAX_NODES];
	Index				   parent			 [MAX_NODES];
	FunctionNodeData	   function_node_data[MAX_NODES];
	Index				   current_backwards_node = NULL_INDEX;
	Index				   next_free_index = 0;
	int					   current_operation = 0;
	vector<EditOperation>  edit_operations;
	double				   time_right_mouse_pressed = 0.;
	int					   last_node_hovered = -1;
	ImNodesMiniMapLocation minimap_location_;

	void clear() {
		memset(used, 0, sizeof(used));
		for (int i = 0; i < MAX_NODES; i++) {
			values[i] = Value::make_value();
			values[i].m_index = NULL_INDEX;
			parent[i] = NULL_INDEX;
		}
		next_free_index = 0;
		current_backwards_node = NULL_INDEX;
		current_operation = 0;
		edit_operations.clear();
	}

	ComputationGraph() {
		clear();
	}

	void collapse_selected_nodes_to_function(const ImVec2& origin, vector<Function>& functions) {
		int num_nodes_selected = ImNodes::NumSelectedNodes();
		int* selected_nodes = nullptr;

		if (num_nodes_selected > 0) {
			selected_nodes = new int[num_nodes_selected];
			ImNodes::GetSelectedNodes(selected_nodes);
			collapse_to_function(selected_nodes, num_nodes_selected, origin, functions);
			delete[] selected_nodes;
		}
	}

	void collapse_to_function(int* indices, size_t num_indices, ImVec2 pos, vector<Function>& functions) {
		if (num_indices <= 0) {
			return;
		}

		Index function_node_index = get_new_value();
		values[function_node_index].m_operation = Operation::Function;

		unordered_set<Index> indices_set;

		for (int i = 0; i < num_indices; i++) {
			indices_set.insert(indices[i]);
		}

		json json_data = to_json(indices, pos, num_indices);

		Function function;
		strcpy(function.m_name, "New function");
		function.m_id = functions.size();
		function.m_json = json_data;
		functions.push_back(function);
		printf("%s", function.m_json.dump(4).c_str());

		int function_id = function.m_id;

		//find inputs

		for (int i = 0; i < num_indices; i++) {
			for (int j = 0; j < NUM_INPUTS; j++) {
				if (values[indices[i]].m_inputs[j].index != NULL_INDEX) {
					if (indices_set.find(values[indices[i]].m_inputs[j].index) == indices_set.end()) {
						Connection connection;
						connection.index = values[indices[i]].m_index;
						connection.input_slot = j;
						function_node_data[function_node_index].m_function_input_nodes.push_back(connection);
					}
				}
			}
		}

		for (int i = 0; i < MAX_NODES; i++) {
			if (used[i]) {
				if (indices_set.find(values[i].m_index) == indices_set.end()) {
					for (int j = 0; j < NUM_INPUTS; j++) {
						if (indices_set.find(values[i].m_inputs[j].index) != indices_set.end()) {
							Connection connection;
							connection.index = values[i].m_inputs[j].index;
							function_node_data[function_node_index].m_function_output_nodes.push_back(connection);
						}
					}
				}
			}
		}

		for (int i = 0; i < num_indices; i++) {
			values[indices[i]].m_parent = function_node_index;
		}

		function_node_data[function_node_index].m_function_id = function_id;
	}

	Index get_new_value() {
		used[next_free_index] = true;
		values[next_free_index].m_index = next_free_index;
		return next_free_index++;
	}

	void delete_value_and_return_removed_connections(Index index, vector<Connection>& removed_connections) {
		if (index == current_backwards_node) {
			current_backwards_node = NULL_INDEX;
		}

		for (int i = 0; i < MAX_NODES; i++) {
			for (int j = 0; j < NUM_INPUTS; j++) {
				if (values[i].m_inputs[j].index == index) {
					removed_connections.push_back(values[i].m_inputs[j]);
					values[i].m_inputs[j] = Connection();
				}
			}
		}

		values[index] = Value();
		return;
	}

	void forwards() {
		//first we find all the nodes with no parents
		unordered_set <Index> checked;
		unordered_set <Index> has_parent;

		for (int i = 0; i < MAX_NODES; i++) {
			if (used[i]) {
				checked.insert(i);
				for (int i = 0; i < NUM_INPUTS; i++) {
					has_parent.insert(values[i].m_inputs[i].index);
				}
			}
		}

		unordered_set <Index> no_parent;

		for (auto& it : checked) {
			if (has_parent.find(it) == has_parent.end()) {
				no_parent.insert(it);
			}
		}

		unordered_set <Index> calculated;
		for (const auto& it : no_parent) {
			vector<Index> sorted_descendants = values[it].get_topological_sorted_descendants(values);

			for (const auto& descendant : sorted_descendants) {
				if (calculated.find(descendant) == calculated.end()) {
					values[descendant].single_forwards(values);
					calculated.insert(descendant);
				}
			}
		}

	}

	void zero_gradients() {
		for (int i = 0; i < MAX_NODES; i++) {
			if (used[i])
				values[i].m_gradient = 0.f;
		}
	}

	Value& get_value(Index index) {
		return values[index];
	}

	void from_json(const json& json, const ImVec2& origin) {
		if (json["nodes"].size() == 0) {
			return;
		}

		map<Index, Index> json_index_to_index;
		for (int i = 0; i < json["nodes"].size(); i++) {
			Index index = get_new_value();
			json_index_to_index[json["nodes"][i]["index"]] = index;
		}

		for (int i = 0; i < json["nodes"].size(); i++) {
			Index json_index = json["nodes"][i]["index"];
			Value value;
			value.from_json(json["nodes"][i]);
			value.m_index = json_index_to_index[json_index];
			for (int j = 0; j < NUM_INPUTS; j++) {
				if (json_index_to_index.find(value.m_inputs[j].index) == json_index_to_index.end()) {
					value.m_inputs[j].index = NULL_INDEX;
				}
				else {
					value.m_inputs[j].index = json_index_to_index[value.m_inputs[j].index];
				}
			}
			EditOperation op = EditOperation::add_node(value, origin + ImVec2(json["offsets"][i][0], json["offsets"][i][1]), false);
			apply_operation(op);
		}

		edit_operations.back().m_final = true;		
	}

	json to_json(int* indices, const ImVec2& origin, int num) {
		int next_json_index = 0;
		nlohmann::json j;
		map<Index, int> index_to_json_index;
		for (int i = 0; i < num; i++) {
			j["nodes"].push_back(values[indices[i]].to_json());
			ImVec2 pos = ImNodes::GetNodeGridSpacePos(indices[i]);
			j["offsets"].push_back({ pos.x - origin.x, pos.y - origin.y });
		}
		return j;
	}

	void add_operation_without_applying(EditOperation& operation) {
		if (current_operation < edit_operations.size()) {
			edit_operations.resize(current_operation);
		}
		edit_operations.push_back(operation);
		current_operation++;
	}

	void apply_operation(EditOperation& operation) {
		if (current_operation < edit_operations.size()) {
			edit_operations.resize(current_operation);
		}
		edit_operations.push_back(operation);
		edit_operations.back().apply(this);
		current_operation++;
	}

	void undo() {
		if (current_operation > 0) {
			do {
				current_operation--;
				edit_operations[current_operation].undo(this);
			} while (current_operation > 0 && !edit_operations[current_operation-1].m_final);
		}
	}

	void redo() {
		if (current_operation < edit_operations.size()) {
			do {
				edit_operations[current_operation].apply(this);
				current_operation++;
			} while (current_operation < edit_operations.size() && !edit_operations[current_operation-1].m_final);
		}
	}

	void copy_selected_nodes(const ImVec2& origin) {
		int num_nodes_selected = ImNodes::NumSelectedNodes();
		int* selected_nodes = nullptr;

		if (num_nodes_selected > 0) {
			selected_nodes = new int[num_nodes_selected];
			ImNodes::GetSelectedNodes(selected_nodes);
			auto json = to_json(selected_nodes, origin, num_nodes_selected);
			clip::set_text(json.dump());
			delete[] selected_nodes;
		}
	}

	void paste(const ImVec2& origin) {
		if (clip::has(clip::text_format())) {
			std::string clipboard;
			clip::get_text(clipboard);
			auto json = json::parse(clipboard);
			from_json(json, origin);
		}
	}

	void delete_selected_nodes() {
		int num_nodes_selected = ImNodes::NumSelectedNodes();
		int* selected_nodes = nullptr;

		if (num_nodes_selected > 0) {
			selected_nodes = new int[num_nodes_selected];
			ImNodes::GetSelectedNodes(selected_nodes);

			for (int i = 0; i < num_nodes_selected; i++) {
				for (int j = 0; j < MAX_NODES; j++) {
					for (int k = 0; k < NUM_INPUTS; k++) {
						if (values[j].m_inputs[k].index == selected_nodes[i]) {
							EditOperation op = EditOperation::remove_link(values[j].m_inputs[k], j, false);
							apply_operation(op);
						}
					}
				}
			}

			for (int i = 0; i < num_nodes_selected; i++) {
				for (int j = 0; j < NUM_INPUTS; j++) {
					if (values[selected_nodes[i]].m_inputs[j].index != NULL_INDEX) {
						EditOperation op = EditOperation::remove_link(values[selected_nodes[i]].m_inputs[j], selected_nodes[i], false);
						apply_operation(op);
					}
				}
			}

			for (int i = 0; i < num_nodes_selected; i++) {
				EditOperation op = EditOperation::remove_node(selected_nodes[i], i == num_nodes_selected - 1);
				apply_operation(op);
			}
		}
	}

	void save(const char* filename) {
		FILE* save_file = fopen(filename, "w");
		unsigned node_count = 0;
		for (int i = 0; i < MAX_NODES; i++) {
			if (used[i]) {
				node_count++;
			}
		}

		int* indices = new int[node_count];
		
		node_count = 0;
		for (int i = 0; i < MAX_NODES; i++) {
			if (used[i]) {
				indices[node_count++] = values[i].m_index;
			}
		}

		auto json = to_json(indices, ImVec2(), node_count);
		
		delete[] indices;
		std::string dump = json.dump(4);
		fwrite(dump.c_str(), 1, dump.size(), save_file);
		fclose(save_file);
	}

	void load(const char* filename) {
		FILE* save_file = fopen(filename, "rb");

		if (save_file) {
			fseek(save_file, 0, SEEK_END);
			long size = ftell(save_file);
			fseek(save_file, 0, SEEK_SET);
			char* buffer = new char[size + 1];
			fread(buffer, 1, size, save_file);
			buffer[size] = 0;
			fclose(save_file);
			auto json = json::parse(buffer);
			delete[] buffer;
			clear();
			from_json(json, ImVec2());
		}
	}

	unsigned get_attribute_input_index(Index i, unsigned input) {
		if (values[i].m_parent == NULL_INDEX) {
			return i * MAX_CONNECTIONS_PER_NODE + input;
		}
		else {
			Value& currentValue = values[i];

			for (int input_pin = 0; input_pin < function_node_data[currentValue.m_parent].m_function_input_nodes.size(); input_pin++) {
				if (function_node_data[currentValue.m_parent].m_function_input_nodes[input_pin].index == currentValue.m_index &&
					function_node_data[currentValue.m_parent].m_function_input_nodes[input_pin].input_slot == currentValue.m_inputs[input].input_slot) {
					return currentValue.m_parent * MAX_CONNECTIONS_PER_NODE + input_pin;
					//ImNodes::Link(currentValue.m_parent * MAX_CONNECTIONS_PER_NODE + input_pin,
						//currentValue.m_inputs[input].index * MAX_CONNECTIONS_PER_NODE + values[currentValue.m_inputs[input].index].m_inputs.size(),
				}
			}
		}

	}

	unsigned get_attribute_output_index(Index i, unsigned output) {
		if (values[i].m_parent == NULL_INDEX) {
			return i * MAX_CONNECTIONS_PER_NODE + NUM_INPUTS;
		}
		else {
			Index parent_index = values[i].m_parent;
			Value& parentValue = values[parent_index];
			for (int output_pin = 0; output_pin < function_node_data[parent_index].m_function_output_nodes.size(); output_pin++) {
				if (function_node_data[parent_index].m_function_output_nodes[output_pin].index == i &&
					function_node_data[parent_index].m_function_output_nodes[output_pin].output_slot == output) {
					return parentValue.m_index * MAX_CONNECTIONS_PER_NODE +
						   function_node_data[parent_index].m_function_input_nodes.size() +
						   output_pin;
				}
			}
		}
	}

	void show(const int editor_id, bool* open, std::vector<Function>& functions, const char* name) {
		forwards();

		zero_gradients();

		if (current_backwards_node != NULL_INDEX)
			get_value(current_backwards_node).backwards(values);

		auto flags = ImGuiWindowFlags_MenuBar;

		// The node editor window

		if (*open) {
			if (ImGui::Begin(name, open, flags)) {
				if (ImGui::BeginMenuBar())
				{
					if (ImGui::BeginMenu("View"))
					{
						if (ImGui::MenuItem("Reset Panning"))
							ImNodes::EditorContextResetPanning(ImVec2());

						if (ImGui::MenuItem("Reset Zoom"))
							ImNodes::EditorContextSetZoom(1.0f);

						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Mini-map"))
					{
						const char* names[] = {
							"Top Left",
							"Top Right",
							"Bottom Left",
							"Bottom Right",
						};
						int locations[] = {
							ImNodesMiniMapLocation_TopLeft,
							ImNodesMiniMapLocation_TopRight,
							ImNodesMiniMapLocation_BottomLeft,
							ImNodesMiniMapLocation_BottomRight,
						};

						for (int i = 0; i < 4; i++)
						{
							bool selected = minimap_location_ == locations[i];
							if (ImGui::MenuItem(names[i], NULL, &selected))
								minimap_location_ = locations[i];
						}
						ImGui::EndMenu();
					}

					ImGui::EndMenuBar();
				}

				ImNodes::BeginNodeEditor(editor_id);
				ImNodes::StyleColorsDark();
				for (int i = 0; i < MAX_NODES; i++) {
					
					Value& currentValue = values[i];

					if (currentValue.m_index != NULL_INDEX) {
						if (currentValue.m_parent == NULL_INDEX) {
							const float node_width = 70.0f;

							if (i == current_backwards_node) {
								ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(191, 109, 11, 255));
								ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(194, 126, 45, 255));
								ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(204, 148, 81, 255));
							}

							if (currentValue.m_operation == Operation::None)
							{
								ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(0x69, 0x0f, 0x62, 255));
								ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(0x8b, 0x26, 0x84, 255));
								ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(0x94, 0x2e, 0x8c, 255));
							}

							ImNodes::BeginNode(i);

							ImNodes::BeginNodeTitleBar();
							switch (currentValue.m_operation) {
							case Operation::Function:
							{
								short function_id = function_node_data[i].m_function_id;
								ImGui::TextUnformatted(functions[function_id].m_name);
							}
								break;
							case Operation::Add:
								ImGui::TextUnformatted("add");
								break;
							case Operation::Subtract:
								ImGui::TextUnformatted("sub");
								break;
							case Operation::Multiply:
								ImGui::TextUnformatted("mul");
								break;
							case Operation::Divide:
								ImGui::TextUnformatted("div");
								break;
							case Operation::Power:
								ImGui::TextUnformatted("pow");
								break;
							default:
								ImGui::TextUnformatted("value");
							}
							ImNodes::EndNodeTitleBar();
							unsigned attribute_index = i * MAX_CONNECTIONS_PER_NODE;

							if (currentValue.m_operation != Operation::Function) {
								for (int input = 0; input < NUM_INPUTS || input < 2; input++) {
									if (currentValue.m_operation != Operation::None) {
										ImNodes::BeginInputAttribute(attribute_index + input);
									}

									if (input == 0) {
										if (currentValue.m_operation == Operation::None) {
											const float label_width = ImGui::CalcTextSize("value").x;

											ImGui::PushItemWidth(node_width - label_width);
											ImGui::DragFloat(
												"##hidelabel", &currentValue.m_value, 0.01f);
											ImGui::PopItemWidth();
										}
										else {
											char text[128];
											sprintf(text, "%.1f", currentValue.m_value);

											const float label_width = ImGui::CalcTextSize(text).x;
											ImGui::Indent(node_width - label_width);
											ImGui::Text(text);
										}
									}

									if (input == 1) {
										if (currentValue.m_operation != Operation::Function) {
											char text[128];
											sprintf(text, "grad %.1f", currentValue.m_gradient);

											const float label_width = ImGui::CalcTextSize(text).x;
											ImGui::Indent(node_width - label_width);
											ImGui::Text(text, currentValue.m_gradient);
										}
									}
									if (currentValue.m_operation != Operation::None) {
										ImNodes::EndInputAttribute();
									}
								}

								ImNodes::BeginOutputAttribute(attribute_index + NUM_INPUTS);
								ImNodes::EndOutputAttribute();

							}
							else {
								for (int input = 0; input < function_node_data[i].m_function_input_nodes.size(); input++) {
									ImNodes::BeginInputAttribute(attribute_index + input);
									ImGui::Text("%i", input);
									ImNodes::EndInputAttribute();
								}

								for (int output = 0; output < function_node_data[i].m_function_output_nodes.size(); output++) {
									ImNodes::BeginOutputAttribute(attribute_index +
																	function_node_data[i].m_function_input_nodes.size() + output);
									ImNodes::EndOutputAttribute();
								}
							}

							if (i == current_backwards_node || currentValue.m_operation == Operation::None) {
								ImNodes::PopColorStyle();
								ImNodes::PopColorStyle();
								ImNodes::PopColorStyle();
							}

							ImNodes::EndNode();
						}

						for (int input = 0; input < NUM_INPUTS; input++) {
							if (currentValue.m_inputs[input].index != NULL_INDEX) {
								if (currentValue.m_parent == NULL_INDEX ||
									values[currentValue.m_inputs[input].index].m_parent == NULL_INDEX ||
									currentValue.m_parent != values[currentValue.m_inputs[input].index].m_parent) {
									unsigned input_index = get_attribute_input_index(i, input);
									unsigned output_index = get_attribute_output_index(currentValue.m_inputs[input].index, currentValue.m_inputs[input].output_slot);

									ImNodes::Link(input_index,
										output_index,
										input_index);
								}
							}
						}
					}
				}

				ImNodes::MiniMap(0.2f, minimap_location_);
				ImNodes::EndNodeEditor();

				if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
					if (ImGui::IsKeyDown(ImGuiKey_ModCtrl)) {
						if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
							undo();
						}
					}

					if (ImGui::IsKeyPressed(ImGuiKey_R)) {
						redo();
					}

					if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
						delete_selected_nodes();
					}
				}

				if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
				{
					auto zoom = ImNodes::EditorContextGetZoom() + ImGui::GetIO().MouseWheel * 0.1f;
					ImNodes::EditorContextSetZoom(zoom, ImGui::GetMousePos());
				}

				int num_nodes_selected = ImNodes::NumSelectedNodes();
				int* selected_nodes = nullptr;

				if (num_nodes_selected > 0) {
					selected_nodes = new int[num_nodes_selected];
					ImNodes::GetSelectedNodes(selected_nodes);
				}

				bool nodes_selected = num_nodes_selected > 0;
				bool node_hovered = ImNodes::IsNodeHovered(&last_node_hovered);
				bool is_hovered_node_selected = false;

				for (int i = 0; i < num_nodes_selected; ++i) {
					is_hovered_node_selected |= (last_node_hovered == selected_nodes[i]);
				}

				if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
					ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					time_right_mouse_pressed = ImGui::GetTime();
				}

				if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
					ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
					ImGui::GetTime() - time_right_mouse_pressed < 0.3) {
					if (!nodes_selected && !node_hovered)
						ImGui::OpenPopup("none selected");
					else if ((node_hovered && !nodes_selected) || (node_hovered && !is_hovered_node_selected))
						ImGui::OpenPopup("node hovered");
					else if (nodes_selected)
						ImGui::OpenPopup("multiple nodes selected");

				}

				if (ImGui::BeginPopup("none selected"))
				{
					const ImVec2 screen_click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();
					ImVec2 click_pos = ImNodes::ScreenSpaceToGridSpace(screen_click_pos);

					if (ImGui::MenuItem("Create Value Node")) {
						Value value = Value::make_value();
						EditOperation edit_operation = EditOperation::add_node(value, click_pos);
						apply_operation(edit_operation);
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Create Add Node")) {
						Value value = Value::make_add();
						EditOperation edit_operation = EditOperation::add_node(value, click_pos);
						apply_operation(edit_operation);
					}
					if (ImGui::MenuItem("Create Subtract Node")) {
						Value value = Value::make_subtract();
						EditOperation edit_operation = EditOperation::add_node(value, click_pos);
						apply_operation(edit_operation);
					}
					if (ImGui::MenuItem("Create Multiply Node")) {
						Value value = Value::make_multiply();
						EditOperation edit_operation = EditOperation::add_node(value, click_pos);
						apply_operation(edit_operation);
					}
					if (ImGui::MenuItem("Create Divide Node")) {
						Value value = Value::make_divide();
						EditOperation edit_operation = EditOperation::add_node(value, click_pos);
						apply_operation(edit_operation);
					}
					if (ImGui::MenuItem("Create Power Node")) {
						Value value = Value::make_power();
						EditOperation edit_operation = EditOperation::add_node(value, click_pos);
						apply_operation(edit_operation);
					}

					if (clip::has(clip::text_format())) {
						ImGui::Separator();
						if (ImGui::MenuItem("Paste")) {
							paste(click_pos);
						}
					}

					ImGui::EndPopup();
				}

				if (ImGui::BeginPopup("multiple nodes selected")) {
					const ImVec2 screen_click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();
					ImVec2 click_pos = ImNodes::ScreenSpaceToGridSpace(screen_click_pos);

					if (ImGui::MenuItem("Delete")) {
						delete_selected_nodes();
					}
					if (ImGui::MenuItem("Copy")) {
						copy_selected_nodes(click_pos);
					}
					if (ImGui::MenuItem("Collapse to Function")) {
						collapse_selected_nodes_to_function(click_pos, functions);
					}

					ImGui::EndPopup();
				}

				if (ImGui::BeginPopup("node hovered")) {
					if (ImGui::MenuItem("Backward Pass")) {
						EditOperation op = EditOperation();
						op.m_index = last_node_hovered;
						op.m_type = EditOperationType::SetBackwardsNode;
						op.m_final = true;
						apply_operation(op);
					}

					if (ImGui::MenuItem("Delete Node")) {
						EditOperation op = EditOperation::remove_node(last_node_hovered);
						apply_operation(op);
					}

					ImGui::EndPopup();
				}

				if (selected_nodes)
					delete[] selected_nodes;

				int num_moved = ImNodes::NumLastMovedNodes();
				if (num_moved > 0) {
					int* moved_nodes = new int[num_moved];
					ImVec2* deltas = new ImVec2[num_moved];
					ImNodes::GetLastMovedNodes(moved_nodes, deltas);
					for (int i = 0; i < num_moved; i++) {
						EditOperation op = EditOperation::move_node(moved_nodes[i], deltas[i], false);
						if (i == num_moved - 1)
							op.m_final = true;
						add_operation_without_applying(op);
					}
					delete[] moved_nodes;
					delete[] deltas;
				}

				int start_attr, end_attr;
				if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
				{
					Index firstNode = start_attr / MAX_CONNECTIONS_PER_NODE;
					int firstNodeAttr = start_attr % MAX_CONNECTIONS_PER_NODE;

					Index secondNode = end_attr / MAX_CONNECTIONS_PER_NODE;
					int secondNodeAttr = end_attr % MAX_CONNECTIONS_PER_NODE;

					bool is_first_input = firstNodeAttr < NUM_INPUTS;
					bool is_second_input = secondNodeAttr < NUM_INPUTS;

					if (is_first_input && !is_second_input) {
						secondNodeAttr -= NUM_INPUTS;
						Connection connection = Connection();
						connection.index = secondNode;
						connection.input_slot = firstNodeAttr;
						connection.output_slot = secondNodeAttr;
						apply_operation(EditOperation::add_link(connection, firstNode));
					}
					else if (!is_first_input && is_second_input) {
						firstNodeAttr -= NUM_INPUTS;
						Connection connection = Connection();
						connection.index = firstNode;
						connection.input_slot = secondNodeAttr;
						connection.output_slot = firstNodeAttr;
						apply_operation(EditOperation::add_link(connection, secondNode));
					}
				}
			}
			ImGui::End();
		}
	}
	bool selected = false;
	
};

void EditOperation::apply(ComputationGraph* context) {
	switch (m_type) {
	case EditOperationType::AddNode:
		{
			Index index = m_value.m_index;
			if (index == NULL_INDEX)
				index = context->get_new_value();
			context->values[index] = m_value;
			context->values[index].m_index = index;
			m_value.m_index = index;
			m_index = index;
			ImNodes::SetNodeGridSpacePos(index, m_position);
		}
		break;
	case EditOperationType::RemoveNode:
		m_value = context->values[m_index];
		m_position = ImNodes::GetNodeGridSpacePos(m_index);
		context->values[m_index] = Value();
		context->used[m_index] = false;
		break;
	case EditOperationType::AddLink:
		context->values[m_index].m_inputs[m_connection.input_slot] = m_connection;
		break;
	case EditOperationType::RemoveLink:
		context->values[m_index].m_inputs[m_connection.input_slot].index = NULL_INDEX;
		break;
	case EditOperationType::MoveNodes:
		ImNodes::SetNodeGridSpacePos(m_index, ImNodes::GetNodeGridSpacePos(m_index) + m_pos_delta);
		break;
	case EditOperationType::SetBackwardsNode:
		m_previousIndex = context->current_backwards_node;
		context->current_backwards_node = m_index;
		break;
	default:
		break;
	}
}

void EditOperation::undo(ComputationGraph* context) {
	switch (m_type) {
	case EditOperationType::AddNode:
		context->values[m_index] = Value();
		break;
	case EditOperationType::RemoveNode:
		{
			Index index = m_value.m_index;
			context->values[index] = m_value;
			context->values[index].m_index = index;
			m_value.m_index = index;
			context->used[index] = true;
			ImNodes::SetNodeGridSpacePos(index, m_position);
		}
		break;
	case EditOperationType::AddLink:
		context->values[m_index].m_inputs[m_connection.input_slot].index = NULL_INDEX;
		break;
	case EditOperationType::RemoveLink:
		context->values[m_index].m_inputs[m_connection.input_slot].index = m_connection.index;
		break;
	case EditOperationType::MoveNodes:
		ImNodes::SetNodeGridSpacePos(m_index, ImNodes::GetNodeGridSpacePos(m_index) - m_pos_delta);
		break;
	case EditOperationType::SetBackwardsNode:
		context->current_backwards_node = m_previousIndex;
		break;
	}
}

EditOperation EditOperation::add_node(const Value& value, const ImVec2& pos, const bool _final) {
	EditOperation op;
	op.m_type = EditOperationType::AddNode;
	op.m_value = value;
	op.m_position = pos;
	op.m_final = _final;
	return op;
}

EditOperation EditOperation::remove_node(const Index index, const bool _final) {
	EditOperation op;
	op.m_type = EditOperationType::RemoveNode;
	op.m_index = index;
	op.m_final = _final;
	return op;
}

EditOperation EditOperation::add_link(const Connection& connection, 
									  const Index index, const bool _final) {
	EditOperation op;
	op.m_type = EditOperationType::AddLink;
	op.m_index = index;
	op.m_connection = connection;
	op.m_final = _final;
	return op;
}

EditOperation EditOperation::remove_link(const Connection& connection,
										 const Index index, const bool _final) {
	EditOperation op;
	op.m_type = EditOperationType::RemoveLink;
	op.m_index = index; 
	op.m_connection = connection;
	op.m_final = _final;
	return op;
}

EditOperation EditOperation::move_node(const Index index, const ImVec2& delta, const bool _final) {
	EditOperation op;
	op.m_type = EditOperationType::MoveNodes;
	op.m_index = index;
	op.m_pos_delta = delta;
	op.m_final = _final;
	return op;
}

class Context {
public:
	Index					   current_backwards_node = NULL_INDEX;
	ComputationGraph		   main_graph;
	map<int, ComputationGraph> function_graphs;
	std::vector<Function>	   functions;

	Context() {
		//clear();
	}

	void clear() {
		//functions.clear();
		//main_graph.clear();
		//for (auto& f : function_graphs) {
		//	f.clear();
		//}
		//function_graphs.clear();
		//current_backwards_node = NULL_INDEX;
	}

	int create_function(const json& json_data) {
	
	}

	void show_function_list(bool* open) {
		if (*open) {
			if (ImGui::Begin("Function List", open)) {
				for (int i = 0; i < functions.size(); i++) {
					ImGui::InputText("##", functions[i].m_name, 128);
					ImGui::SameLine();
					ImGui::MenuItem("Open", "", &functions[i].m_is_open);
				}
				ImGui::End();
			}
		}
	}

	void show(bool* open) {
		main_graph.show(0, open, functions, "main graph");
		for (int function_id = 0; function_id < functions.size(); function_id++) {
			if (functions[function_id].m_is_open) {
				if (function_graphs.find(function_id) == function_graphs.end()) {
					ImNodes::CreateContext(function_id + 1);
					function_graphs[function_id].clear();
					function_graphs[function_id].from_json(functions[function_id].m_json, ImVec2());
				}
				char name[128];
				sprintf(name, "Function ID %i", function_id);
				function_graphs[function_id].show(function_id+1, &functions[function_id].m_is_open, functions, name);
			}
			else {
				if (function_graphs.find(function_id) != function_graphs.end()) {
					function_graphs[function_id].clear();
					function_graphs.erase(function_id);
				}
			}
		}
	}

	void save(const char* filename) {
		main_graph.save(filename);
	}

	void load(const char* filename) {
		main_graph.load(filename);
	}
};

Context s_context;

void show_function_list(bool* open) {
	s_context.show_function_list(open);
}

void show_graph_editor(bool* open) {
	s_context.show(open);
}

void save(const char* filename) {
	s_context.save(filename);
}

void load(const char* filename) {
	s_context.load(filename);
}
