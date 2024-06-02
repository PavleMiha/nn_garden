#include "computation_graph.h"
#include "bimg/bimg.h"
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>

using nlohmann::json;

void ComputationGraph::clear() {
	memset(used, 0, sizeof(used));
	for (int i = 0; i < MAX_NODES; i++) {
		values[i] = Value::make_value();
		values[i].m_index = NULL_INDEX;
		parent[i] = NULL_INDEX;
	}
	next_free_index = 0;
	current_backwards_node = NULL_INDEX;
	current_result_node = NULL_INDEX;
	current_operation = 0;
	edit_operations.clear();
}

void ComputationGraph::collapse_selected_nodes_to_new_function(const ImVec2& origin, vector<Function>& functions) {
	int num_nodes_selected = ImNodes::NumSelectedNodes();
	Index* selected_nodes = nullptr;

	if (num_nodes_selected > 0) {
		selected_nodes = new Index[num_nodes_selected];
		ImNodes::GetSelectedNodes((int*)selected_nodes);
		collapse_to_new_function(selected_nodes, num_nodes_selected, origin, functions);
		delete[] selected_nodes;
	}
}

void ComputationGraph::collapse_to_new_function(Index* indices, size_t num_indices, ImVec2 pos, vector<Function>& functions) {
	json json_data = to_json(indices, pos, num_indices);

	Function function;
	strcpy(function.m_name, "New function");
	function.m_id = functions.size();
	function.m_json = json_data;
	function.m_num_inputs = function.m_json["unmatched_inputs"].size();
	function.m_num_outputs = function.m_json["unmatched_outputs"].size();
	functions.push_back(function);
	printf("FUNCTION: %s", function.m_json.dump(4).c_str());

	int function_id = function.m_id;

	unordered_set<Index> indices_set;

	for (int i = 0; i < num_indices; i++) {
		indices_set.insert(indices[i]);
	}

	Index function_node_index = collapse_to_function(indices, num_indices, pos, function_id);

	for (int i = 0; i < num_indices; i++) {
		for (int j = 0; j < MAX_INPUTS; j++) {
			if (values[indices[i]].m_inputs[j].node != NULL_INDEX) {
				if (indices_set.find(values[indices[i]].m_inputs[j].node) == indices_set.end()) {
					Socket socket;
					socket.node   = indices[i];
					socket.slot   = j;

					function_node_data[function_node_index].m_function_input_nodes.push_back(socket);
				}
			}
		}
	}

	for (int i = 0; i < MAX_NODES; i++) {
		if (used[i]) {
			if (indices_set.find(i) == indices_set.end()) {
				for (int j = 0; j < MAX_INPUTS; j++) {
					if (indices_set.find(values[i].m_inputs[j].node) != indices_set.end()) {
						Socket socket;
						socket= values[i].m_inputs[j];
						function_node_data[function_node_index].m_function_output_nodes.push_back(socket);
					}
				}
			}
		}
	}

}

Index ComputationGraph::collapse_to_function(Index* indices, size_t num_indices, ImVec2 pos, int function_id) {
	if (num_indices <= 0) {
		return NULL_INDEX;
	}

	ImVec2 average_pos = ImVec2(0, 0);

	for (int i = 0; i < num_indices; i++) {
		average_pos.x += values[indices[i]].m_position.x;
		average_pos.y += values[indices[i]].m_position.y;
	}

	average_pos.x /= num_indices;
	average_pos.y /= num_indices;

	Index function_node_index = get_new_value();
	values[function_node_index].m_operation = Operation::Function;
	values[function_node_index].m_position = average_pos;
	
	for (int i = 0; i < num_indices; i++) {
		values[indices[i]].m_parent = function_node_index;
	}

	function_node_data[function_node_index].m_function_id = function_id;

	return function_node_index;
}

Index ComputationGraph::instantiate_function(int function_id, const ImVec2& pos, const vector<Function>& functions) {
	FromJsonAdditionalData additional_data;
	from_json(functions[function_id].m_json, pos, &additional_data);

	Index i = collapse_to_function(&(additional_data.indices[0]), additional_data.indices.size(), pos, functions[function_id].m_id);

	function_node_data[i].m_function_input_nodes = additional_data.unmatched_inputs;
	function_node_data[i].m_function_output_nodes = additional_data.unmatched_outputs;
	return i;
}


Index ComputationGraph::get_new_value() {
	used[next_free_index] = true;
	values[next_free_index].m_index = next_free_index;
	return next_free_index++;
}

void ComputationGraph::delete_value_and_return_removed_connections(Index index, vector<Connection>& removed_connections) {
	for (int i = 0; i < MAX_NODES; i++) {
		for (int j = 0; j < MAX_INPUTS; j++) {
			if (values[i].m_inputs[j].node == index) {
				Connection connection;
				connection.start = values[i].m_inputs[j];
				connection.end.node = i;
				connection.end.slot = j;
				removed_connections.push_back(connection);
				values[i].m_inputs[j] = Socket();
			}
		}
	}

	used[index] = false;

	if (values[index].m_name != nullptr) {
		free(values[index].m_name);
	}

	values[index] = Value();
	return;
}

std::default_random_engine generator;
std::uniform_real_distribution<double> distribution(-1.0, 1.0);

void ComputationGraph::randomize_parameters() {
	for (int i = 0; i < MAX_NODES; i++) {
		if (used[i]) {
			if (values[i].m_operation == Operation::Parameter) {
				values[i].m_value = distribution(generator);
			}
		}
	}
}

void ComputationGraph::do_stochastic_gradient_descent_step(float learning_rate) {
	for (int i = 0; i < MAX_NODES; i++) {
		if (used[i]) {
			if (values[i].m_operation != Operation::Parameter && values[i].m_operation != Operation::Constant) {
				values[i].m_value = 0.f;
			}
			values[i].m_gradient = 0.f;
			values[i].m_gradient_calculated = false;
		}
	}
	forwards(&data_source.data[data_source.current_data_point].x);

	if (current_backwards_node != NULL_INDEX)
		backwards(&data_source.data[data_source.current_data_point].x);

	for (int i = 0; i < MAX_NODES; i++) {
		if (used[i]) {
			if (values[i].m_operation == Operation::Parameter) {
				values[i].m_value -= learning_rate * values[i].m_gradient;
			}
		}
	}
}

void ComputationGraph::do_stochastic_gradient_descent(float learning_rate, int batch_size, int& current_point, const vector<int> shuffled_points) {
	memset(gradient_acc, 0, sizeof(gradient_acc));

	for (int i = 0; i < batch_size; i++) {
		data_source.current_data_point = shuffled_points[current_point];
		current_point = (current_point+1)%shuffled_points.size();
		forwards(&data_source.data[data_source.current_data_point].x);
		if (current_backwards_node != NULL_INDEX)
			backwards(&data_source.data[data_source.current_data_point].x);
		for (int i = 0; i < next_free_index; i++) {
			if (used[i]) {
				if (values[i].m_operation == Operation::Parameter) {
					gradient_acc[i] += values[i].m_gradient;
				}
			}
		}
	}

	for (int i = 0; i < next_free_index; i++) {
		if (used[i]) {
			float rate = learning_rate / (float)batch_size;
			if (values[i].m_operation == Operation::Parameter) {
				values[i].m_value -= rate * gradient_acc[i];
			}
		}
	}

}


void ComputationGraph::forwards(float* data_values) {
	//first we find all the nodes with no parents
	if (cached_topological_sorted_descendants.size() == 0) {
		unordered_set <Index> checked;
		unordered_set <Index> has_parent;

		for (int i = 0; i < next_free_index; i++) {
			if (used[i]) {
				checked.insert(i);
				for (int k = 0; k < MAX_INPUTS; k++) {
					if (values[i].m_inputs[k].node != NULL_INDEX)
						has_parent.insert(values[i].m_inputs[k].node);
				}
			}
		}

		for (auto& it : checked) {
			if (has_parent.find(it) == has_parent.end()) {
				cached_no_parent.insert(it);
			}
		}

		unordered_set <Index> calculated;
		for (const auto& it : cached_no_parent) {
			vector<Index> sorted_descendants = values[it].get_topological_sorted_descendants(values);
			cached_topological_sorted_descendants[it] = sorted_descendants;

			
		}
	}

	for (const auto& it : cached_no_parent) {
		for (const auto& descendant : cached_topological_sorted_descendants[it]) {
			values[descendant].single_forwards(values, data_values);
		}

	}
}

void ComputationGraph::backwards(float* data_values) {
	if (cached_sorted_descendants.size() == 0) {
		cached_sorted_descendants = values[current_backwards_node].get_topological_sorted_descendants(values);
	}

	for (auto& it : cached_sorted_descendants) {
		values[it].m_gradient = 0.f;
		values[it].m_gradient_calculated = true;
	}

	values[current_backwards_node].m_gradient = 1.f;

	values[current_backwards_node].m_gradient_calculated = true;

	for (vector<Index>::reverse_iterator it = cached_sorted_descendants.rbegin(); it != cached_sorted_descendants.rend(); ++it) {

		values[*it].single_backwards(values, data_values);
	}
}

void ComputationGraph::zero_gradients() {
	for (int i = 0; i < next_free_index; i++) {
		if (used[i]) {
			values[i].m_gradient = 0.f;
			values[i].m_gradient_calculated = false;
		}
	}
}

Value& ComputationGraph::get_value(Index index) {
	return values[index];
}

vector<Index> ComputationGraph::from_json(const json& json, const ImVec2& origin, FromJsonAdditionalData* additional_data) {
	if (json["nodes"].size() == 0) {
		return vector<Index>();
	}

	vector<Index> nodes = vector<Index>();

	printf("FROM_JSON: %s\n", json.dump(4).c_str());

	map<Index, Index> json_index_to_index;
	for (int i = 0; i < json["nodes"].size(); i++) {
		Index index = get_new_value();
		json_index_to_index[json["nodes"][i]["index"]] = index;
		nodes.push_back(index);
		if (additional_data != nullptr)
		{
			additional_data->indices.push_back(index);

			bool is_unmatched_input = false;
			for (auto ui : json["unmatched_inputs"]) {
				if (ui["end"] == json["nodes"][i]["index"]) {
					is_unmatched_input = true;
					Socket socket;
					socket.node = index;
					socket.slot = ui["end_slot"];

					additional_data->unmatched_inputs.push_back(socket);
				}
			}

			for (auto uo : json["unmatched_outputs"]) {
				if (uo["start"] == json["nodes"][i]["index"]) {
					is_unmatched_input = true;
					Socket socket;
					socket.node = index;
					socket.slot = uo["start_slot"];

					additional_data->unmatched_outputs.push_back(socket);
				}
			}
		}
	}

	for (int i = 0; i < json["nodes"].size(); i++) {
		Index json_index = json["nodes"][i]["index"];
		Value value;
		value.from_json(json["nodes"][i]);
		value.m_index = json_index_to_index[json_index];
		if (value.m_parent != NULL_INDEX) {
			value.m_parent = json_index_to_index[value.m_parent];
		}
		for (int j = 0; j < MAX_INPUTS; j++) {
			if (json_index_to_index.find(value.m_inputs[j].node) == json_index_to_index.end()) {
				value.m_inputs[j] = Socket();
			}
			else {
				value.m_inputs[j].node = json_index_to_index[value.m_inputs[j].node];
			}
		}

		if (value.m_parent == NULL_INDEX) {
			value.m_position = origin + ImVec2(json["offsets"][i][0], json["offsets"][i][1]);
		}

		EditOperation op = EditOperation::add_node(value, false);
		apply_operation(op);
	}

	for (int i = 0; i < json["function_node_data"].size(); i++) {
		if (json["function_node_data"][i].is_null())
			continue;
		function_node_data[json_index_to_index[i]].m_function_id = json["function_node_data"][i]["function_id"];
		for (auto& input : json["function_node_data"][i]["function_input_nodes"]) {
			Socket socket;
			socket.from_json(input);
			socket.node = json_index_to_index[input["node"]];
			function_node_data[json_index_to_index[i]].m_function_input_nodes.push_back(socket);
		}
		for (auto& output : json["function_node_data"][i]["function_output_nodes"]) {
			Socket socket;
			socket.from_json(output);
			socket.node = json_index_to_index[output["node"]];
			function_node_data[json_index_to_index[i]].m_function_output_nodes.push_back(socket);
		}
	}

	edit_operations.back().m_final = true;
	return nodes;
}

json ComputationGraph::to_json(Index* indices, const ImVec2& origin, int num) const {
	int next_json_index = 0;
	json j;
	map<Index, int> index_to_json_index;

	for (int i = 0; i < num; i++) {
		index_to_json_index[indices[i]] = next_json_index++;
	}

	json unmatched_inputs;
	int replacement_input_index = num;

	for (int i = 0; i < num; i++) {
		j["nodes"].push_back(values[indices[i]].to_json());
		j["nodes"][i]["index"] = index_to_json_index[indices[i]];

		if (values[indices[i]].m_parent != NULL_INDEX)
			j["nodes"][i]["parent"] = index_to_json_index[values[indices[i]].m_parent];

		printf("Index %i has parent %i\n", indices[i], values[indices[i]].m_parent);
		if (values[indices[i]].m_parent == NULL_INDEX) {
			ImVec2 pos = ImNodes::GetNodeGridSpacePos(indices[i]);
			j["offsets"].push_back({ pos.x - origin.x, pos.y - origin.y });
		}
		else {
			j["offsets"].push_back({ 0, 0 });
		}
	}

	printf(j.dump(4).c_str());

	for (int i = 0; i < num; i++) {
 		for (auto& it : j["nodes"][i]["inputs"]) {

			if (it.is_null()) {
				continue;
			}

			if (it["node"] != NULL_INDEX) {
				if (index_to_json_index.find((Index)it["node"]) == index_to_json_index.end()) {
					bool found = false;

					//look for an existing unmatched input, in case there's multiple inputs from the same node
					for (auto& ui : unmatched_inputs) {
						if (ui["index"] == it["node"] && ui["output_slot"] == it["slot"]) {
							found = true;
							it["node"] = ui["replacement_index"];
							break;
						}
					}

					if (!found) {
						json unmatched_input;
						unmatched_input["original_start"] = it["node"];
						unmatched_input["original_start_slot"] = it["slot"];
						unmatched_input["end"] = j["nodes"][i]["index"];
						unmatched_input["end_slot"] = it["end_slot"];
						//unmatched_input["replacement_index"] = replacement_input_index;
						unmatched_inputs.push_back(unmatched_input);
						it["node"] = replacement_input_index++;
					}
				}
				else
				{
					it["node"] = index_to_json_index[(Index)it["node"]];
				}
			}
		}
	}

	json unmatched_outputs;
	int unmatched_output_index = 0;
	for (int i = 0; i < next_free_index; i++) {
		if (used[i]) {
			if (index_to_json_index.find(i) == index_to_json_index.end()) {
				for (int j = 0; j < next_free_index; j++) {
					if (values[i].m_inputs[j].node != NULL_INDEX) {
						if (index_to_json_index.find(values[i].m_inputs[j].node) != index_to_json_index.end()) {
							json unmatched_output;
							unmatched_output["start"] = index_to_json_index[values[i].m_inputs[j].node];
							unmatched_output["start_slot"] = values[i].m_inputs[j].slot;
							unmatched_output["original_end"] = i;
							unmatched_output["original_end_slot"] = j;
							unmatched_outputs.push_back(unmatched_output);
						}
					}
				}
			}
		}
	}

	j["unmatched_inputs"] = unmatched_inputs;
	j["unmatched_outputs"] = unmatched_outputs;

	for (int i = 0; i < num; i++) {
		Index index = indices[i];
		int json_index = index_to_json_index[index];
		j["function_node_data"][json_index]["function_id"] = function_node_data[index].m_function_id;
		json input_nodes = json();
		for (auto input : function_node_data[index].m_function_input_nodes) {

			json json_data = json();
			input.node = index_to_json_index[input.node];
			json_data = input.to_json();
			input_nodes.push_back(json_data);
		}

		j["function_node_data"][json_index]["function_input_nodes"] = input_nodes;

		json output_nodes = json();
		for (auto output : function_node_data[index].m_function_output_nodes) {

			json json_data = json();
			output.node = index_to_json_index[output.node];
			json_data = output.to_json();
			output_nodes.push_back(json_data);
		}
		j["function_node_data"][json_index]["function_output_nodes"] = output_nodes;
	}

	printf("TO_JSON: %s\n", j.dump(4).c_str());
	return j;
}

void ComputationGraph::add_operation_without_applying(EditOperation& operation) {
	if (current_operation < edit_operations.size()) {
		edit_operations.resize(current_operation);
	}
	edit_operations.push_back(operation);
	current_operation++;
}

void ComputationGraph::apply_operation(EditOperation& operation) {
	if (current_operation < edit_operations.size()) {
		edit_operations.resize(current_operation);
	}
	edit_operations.push_back(operation);
	edit_operations.back().apply(this);
	cached_no_parent.clear();
	cached_sorted_descendants.clear();
	cached_topological_sorted_descendants.clear();
	current_operation++;
}

void ComputationGraph::undo() {
	if (current_operation > 0) {
		do {
			current_operation--;
			edit_operations[current_operation].undo(this);
		} while (current_operation > 0 && !edit_operations[current_operation - 1].m_final);
	}
}

void ComputationGraph::redo() {
	if (current_operation < edit_operations.size()) {
		do {
			edit_operations[current_operation].apply(this);
			current_operation++;
		} while (current_operation < edit_operations.size() && !edit_operations[current_operation - 1].m_final);
	}
}
#include <algorithm>  
void ComputationGraph::copy_selected_nodes(const ImVec2& origin) {
	int num_nodes_selected = ImNodes::NumSelectedNodes();
	vector<int> selected_nodes = vector<int>(num_nodes_selected);

	if (num_nodes_selected > 0) {
		ImNodes::GetSelectedNodes((int*)&selected_nodes[0]);

		for (int i = 0; i < selected_nodes.size(); i++) {
			Index index = selected_nodes[i]; 
			if (values[index].m_operation == Operation::Function) {
				for (int j = 0; j < MAX_NODES; j++) {
					if (used[j]) {
						if (values[j].m_parent == index) {
							selected_nodes.push_back(j);
						}
					}
				}
			}
		}

		std::sort(selected_nodes.begin(), selected_nodes.end());

		auto json = to_json((Index*)&selected_nodes[0], origin, selected_nodes.size());
		clip::set_text(json.dump());
	}
}

void ComputationGraph::paste(const ImVec2& origin) {
	if (clip::has(clip::text_format())) {
		std::string clipboard;
		clip::get_text(clipboard);
		auto json = json::parse(clipboard);
		nodes_to_select = from_json(json, origin);
	}
}

void ComputationGraph::delete_selected_nodes() {
	int num_nodes_selected = ImNodes::NumSelectedNodes();
	int* selected_nodes = nullptr;

	if (num_nodes_selected > 0) {
		selected_nodes = new int[num_nodes_selected];
		ImNodes::GetSelectedNodes(selected_nodes);

		for (int i = 0; i < num_nodes_selected; i++) {
			for (int j = 0; j < MAX_NODES; j++) {
				for (int k = 0; k < MAX_INPUTS; k++) {
					if (values[j].m_inputs[k].node == selected_nodes[i]) {
						//remove all links
						Connection connection;
						connection.start = values[j].m_inputs[k];
						connection.end.node = j;
						connection.end.slot = k;
						EditOperation op = EditOperation::remove_link(connection, j, false);
						apply_operation(op);
					}
				}
			}
		}

		for (int i = 0; i < num_nodes_selected; i++) {
			for (int j = 0; j < MAX_INPUTS; j++) {
				if (values[selected_nodes[i]].m_inputs[j].node != NULL_INDEX) {
					Connection connection;
					connection.start = values[selected_nodes[i]].m_inputs[j];
					connection.end.node = selected_nodes[i];
					connection.end.slot = j;
					EditOperation op = EditOperation::remove_link(connection, selected_nodes[i], false);
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

json ComputationGraph::to_json() const {
	unsigned node_count = 0;
	for (int i = 0; i < next_free_index; i++) {
		if (used[i]) {
			node_count++;
		}
	}

	Index* indices = new Index[node_count];

	node_count = 0;
	for (int i = 0; i < next_free_index; i++) {
		if (used[i]) {
			indices[node_count++] = values[i].m_index;
		}
	}

	auto json = to_json(indices, ImVec2(), node_count);
	delete[] indices;

	return json;
}

unsigned ComputationGraph::get_attribute_input_index(Index i, unsigned input) {
	if (values[i].m_parent == NULL_INDEX) {
		return i * MAX_CONNECTIONS_PER_NODE + input;
	}
	else {
		Value& currentValue = values[i];

		for (int input_pin = 0; input_pin < function_node_data[currentValue.m_parent].m_function_input_nodes.size(); input_pin++) {
			if (function_node_data[currentValue.m_parent].m_function_input_nodes[input_pin].node == currentValue.m_index &&
				function_node_data[currentValue.m_parent].m_function_input_nodes[input_pin].slot == input) {
				return currentValue.m_parent * MAX_CONNECTIONS_PER_NODE + input_pin;
			}
		}
	}
}

unsigned ComputationGraph::get_attribute_output_index(Index i, unsigned output) {
	if (values[i].m_parent == NULL_INDEX) {
		return i * MAX_CONNECTIONS_PER_NODE + MAX_INPUTS + output;
	}
	else {
		Index parent_index = values[i].m_parent;
		Value& parentValue = values[parent_index];
		for (int output_pin = 0; output_pin < function_node_data[parent_index].m_function_output_nodes.size(); output_pin++) {
			if (function_node_data[parent_index].m_function_output_nodes[output_pin].node == i &&
				function_node_data[parent_index].m_function_output_nodes[output_pin].slot == output) {
				return parentValue.m_index * MAX_CONNECTIONS_PER_NODE + MAX_INPUTS +
					output_pin;
			}
		}
	}
}

bool DataSource::load(const char* filename) {
	std::string line;
	std::ifstream file(filename);

	current_data_point = 0;
	// Check if file is open
	if (!file.is_open()) {
		std::cerr << "Error opening file" << std::endl;
		return false;
	}

	data.clear();
	// Read lines from the CSV file
	while (getline(file, line)) {
		std::stringstream ss(line);
		DataPoint point;
		char comma; // Helper char to skip commas

		// Parse the line
		if (ss >> point.x >> comma >> point.y >> comma >> point.label) {
			if (point.label == 0)
				point.label = -1;
			data.push_back(point); // Add the data point to the vector
		}
	}

	file.close();

	for (int i = 0; i < NUM_IMAGES; i++) {
		update_image(nullptr);
	}
	return 0;
}

void DataSource::update_image(ComputationGraph* graph) {
	current_image_index = (current_image_index + 1) % NUM_IMAGES;
	// Output the data to check
	if (image_data[current_image_index] == nullptr) {
		image_data[current_image_index] = malloc(256 * 256 * 4);
		memset(image_data[current_image_index], 0, 256 * 256 * 4);
		background_image_data[current_image_index] = malloc(BACKGROUND_IMAGE_RESOLUTION * BACKGROUND_IMAGE_RESOLUTION * 4);
	}

	if (image_handle[current_image_index].idx != UINT16_MAX) {
		bgfx::destroy(image_handle[current_image_index]);
		bgfx::destroy(background_image_handle[current_image_index]);
	}

	for (int x = 0; x < 256; x++) {
		for (int y = 0; y < 256; y++) {
			((uint32_t*)image_data[current_image_index])[x + y * 256] = 0x00ffffff;
		}
	}
	min = ImVec2(FLT_MAX, FLT_MAX);
	max = ImVec2(-FLT_MAX, -FLT_MAX);

	for (int i = 0; i < data.size(); i++) {
		min.x = ImMin(min.x, data[i].x);
		min.y = ImMin(min.y, data[i].y);
		max.x = ImMax(max.x, data[i].x);
		max.y = ImMax(max.y, data[i].y);
	}

	min.x -= (max.x - min.x) * 0.1f;
	min.y -= (max.y - min.y) * 0.1f;
	max.x += (max.x - min.x) * 0.1f;
	max.y += (max.y - min.y) * 0.1f;

	if (graph && graph->current_result_node) {
		for (int x = 0; x < BACKGROUND_IMAGE_RESOLUTION; x++) {
			for (int y = 0; y < BACKGROUND_IMAGE_RESOLUTION; y++) {
				override_data_point.x = min.x + (max.x - min.x) * (((float)x+0.5f) / (float)BACKGROUND_IMAGE_RESOLUTION);
				override_data_point.y = (min.y + (max.y - min.y) * (((float)y+0.5f) / (float)BACKGROUND_IMAGE_RESOLUTION));
				should_override_data_point = true;
				graph->forwards(&override_data_point.x);

				float result = graph->values[graph->current_result_node].m_value;

				//unsigned int red = 255 - (100 * ImClamp(result, 0.f, 1.f));
				unsigned int red = 255 - (100 * ImClamp(result, 0.f, 1.f));
				unsigned int green = 255 - (100 * ImAbs(result)/1.5f);
				unsigned int blue = 255 - (100 * ImClamp(-result, 0.f, 1.f));
///todo double buffer and only update background
				((uint32_t*)background_image_data[current_image_index] )[x + (BACKGROUND_IMAGE_RESOLUTION - 1 - y) * BACKGROUND_IMAGE_RESOLUTION] = IM_COL32(red, green, blue, 255);
			}
		}
	}
	else {
		for (int x = 0; x < BACKGROUND_IMAGE_RESOLUTION; x++) {
			for (int y = 0; y < BACKGROUND_IMAGE_RESOLUTION; y++) {
				((uint32_t*)background_image_data[current_image_index])[x + y * BACKGROUND_IMAGE_RESOLUTION] = IM_COL32(255, 255, 255, 255);
			}
		}
	}

	for (int i = 0; i < data.size(); i++) {
		DataPoint& point = data[i];
		float coord_x = (point.x - min.x) / (max.x - min.x);
		float coord_y = 1.f - (point.y - min.y) / (max.y - min.y);

		uint32_t color = (point.label == 1) ? 0xffff0000 : 0xff0000ff;

		color = (i == current_data_point) ? 0xff00ff00 : color;

		const int(*offsets)[2] = point.label == 1 ? x_offsets : cross_offsets;
		for (int i = 0; i < 5; i++) {
			int x = (int)(coord_x * 256.f) + offsets[i][0];
			int y = (int)(coord_y * 256.f) + offsets[i][1];
			x = ImClamp(x, 0, 255);
			y = ImClamp(y, 0, 255);
			((uint32_t*)image_data[current_image_index])[x + y * 256] = color;
		}
	}

	image_mem[current_image_index] = bgfx::makeRef((char*)image_data[current_image_index], 256 * 256 * 4);
	background_image_mem[current_image_index] = bgfx::makeRef((char*)background_image_data[current_image_index], BACKGROUND_IMAGE_RESOLUTION * BACKGROUND_IMAGE_RESOLUTION * 4);

	image_handle[current_image_index] = bgfx::createTexture2D((uint16_t)256, (uint16_t)256, false, 1, bgfx::TextureFormat::RGBA8, 0, image_mem[current_image_index]);

	background_image_handle[current_image_index] = bgfx::createTexture2D((uint16_t)BACKGROUND_IMAGE_RESOLUTION, (uint16_t)BACKGROUND_IMAGE_RESOLUTION, false, 1, bgfx::TextureFormat::RGBA8, 0, background_image_mem[current_image_index]);
}

void DataSource::set_current_data_point(int index) {
	current_data_point = ImClamp(index, 0, (int)data.size());
	//const bgfx::Memory* mem = bgfx::makeRef((char*)data + index*28*28, 28*28);

	//image_handle = bgfx::createTexture2D((uint16_t)28, (uint16_t)28, false, 1, bgfx::TextureFormat::A8, 0, mem);
}

void DataSource::show_body(int attribute_index) {

	if (image_handle[current_image_index].idx != UINT16_MAX) {
		ImVec2 lastPos = ImGui::GetCursorScreenPos();

		ImGui::Image((ImTextureID)background_image_handle[(current_image_index+1)%3].idx, ImVec2(100, 100));
		ImGui::SetCursorScreenPos(lastPos);

		ImGui::Image((ImTextureID)image_handle[(current_image_index + 1) % NUM_IMAGES].idx, ImVec2(100, 100));
	}

	float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
	if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { set_current_data_point(current_data_point - 1); update_image(); }
	ImGui::SameLine(0.0f, spacing);
	if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { set_current_data_point(current_data_point + 1); update_image(); }

	const float node_width = 100.f;

//	ImGui::PushItemWidth(node_width - label_width);

	ImNodes::BeginOutputAttribute(attribute_index);
	char text[128] = {};
	sprintf(text, "%.1f x", data[current_data_point].x);
	float label_width = ImGui::CalcTextSize(text).x;
	ImGui::Indent(node_width - label_width);
	ImGui::Text(text);
	ImNodes::EndOutputAttribute();
	ImNodes::BeginOutputAttribute(attribute_index + 1);
	sprintf(text, "%.1f y", data[current_data_point].y);
	label_width = ImGui::CalcTextSize(text).x;
	ImGui::Indent(node_width - label_width);
	ImGui::Text(text);
	ImNodes::EndOutputAttribute();
	ImNodes::BeginOutputAttribute(attribute_index + 2);
	sprintf(text, "%.0f label", data[current_data_point].label);
	label_width = ImGui::CalcTextSize(text).x;
	ImGui::Indent(node_width - label_width);
	ImGui::Text(text);
	ImNodes::EndOutputAttribute();
}

void ComputationGraph::render_gradient(Index i, float node_width) {
	if (values[i].m_gradient_calculated) {
		char text[128];
		sprintf(text, "%.3f", values[i].m_gradient);
		const float label_width = ImGui::CalcTextSize(text).x;

		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(204, 148, 81, 255));
		ImGui::Indent(node_width - label_width);
		ImGui::Text(text, values[i].m_gradient);
		ImGui::PopStyleColor();
	}
}

void ComputationGraph::show_node(Index i, vector<Function>& functions) {
	Value& currentValue = values[i];

	const float node_width = 70.0f;

	if (currentValue.m_operation == Operation::Backwards) {
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(191, 109, 11, 255));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(194, 126, 45, 255));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(204, 148, 81, 255));
	}

	if (currentValue.m_operation == Operation::Parameter)
	{
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(0x69, 0x0f, 0x62, 255));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(0x8b, 0x26, 0x84, 255));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(0x94, 0x2e, 0x8c, 255));
	}

	if (currentValue.m_operation == Operation::Constant)
	{
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(0x23, 0xa4, 0x42, 255));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(0x51, 0xcb, 0x6b, 255));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(0x7c, 0xd8, 0x92, 255));
	}

	if (currentValue.m_operation == Operation::Result)
	{
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(0x25, 0x96, 0xbe, 255));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(0x66, 0xb6, 0xd2, 255));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(0x7c, 0xc0, 0xd8, 255));
	}

	if (currentValue.m_operation == Operation::Display)
	{
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(0x25, 0x96, 0xbe, 255));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(0x66, 0xb6, 0xd2, 255));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(0x7c, 0xc0, 0xd8, 255));
	}

	//ToDo: This could cause issues if we run it before we ever run begin node for any node
	if (currentValue.m_positionDirty) {
		ImNodes::SetNodeGridSpacePos(currentValue.m_index, currentValue.m_position);
		currentValue.m_positionDirty = false;
	}

	ImNodes::BeginNode(i);

	ImNodes::BeginNodeTitleBar();
	switch (currentValue.m_operation) {
	case Operation::FunctionInput:
		ImGui::TextUnformatted("input");
		break;
	case Operation::FunctionOutput:
		ImGui::TextUnformatted("output");
		break;
	case Operation::Display:
	case Operation::Result:
	case Operation::Backwards:
	{
		if (currentValue.m_name == nullptr) {
			currentValue.m_name = new char[128];
			sprintf(currentValue.m_name, "display");
		}
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushItemWidth(ImGui::CalcTextSize(currentValue.m_name).x + 10.0f);
		int size = strlen(currentValue.m_name);
		if (currentValue.m_operation == Operation::Display)
			ImGui::InputText("##input", currentValue.m_name, 128);
		if (currentValue.m_operation == Operation::Result)
			ImGui::Text("Result");
		if (currentValue.m_operation == Operation::Backwards)
			ImGui::Text("Backwards");
		ImGui::PopItemWidth();
		ImGui::PopStyleColor();
	}
		break;
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
	case Operation::Tanh:
		ImGui::TextUnformatted("tanh");
		break;
	case Operation::ReLU:
		ImGui::TextUnformatted("ReLU");
		break;
	case Operation::Sin:
		ImGui::TextUnformatted("sin");
		break;
	case Operation::Cos:
		ImGui::TextUnformatted("cos");
		break;
	case Operation::Sqrt:
		ImGui::TextUnformatted("sqrt");
		break;
	case Operation::Parameter:
	{
		if (currentValue.m_name == nullptr) {
			currentValue.m_name = new char[128];
			sprintf(currentValue.m_name, "param");
		}
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushItemWidth(glm::max(node_width, ImGui::CalcTextSize(currentValue.m_name).x + 10.0f));
		int size = strlen(currentValue.m_name);
		ImGui::InputText("##input", currentValue.m_name, 128);
		ImGui::PopItemWidth();
		ImGui::PopStyleColor();
	}
		break;
	case Operation::Constant:
		ImGui::TextUnformatted("constant");
		break;
	case Operation::DataSource:
		ImGui::TextUnformatted("data");
		break;
	default:
		IM_ASSERT(0 && "Missing title for operation type");
		break;
	}
	ImNodes::EndNodeTitleBar();
	unsigned attribute_index = i * MAX_CONNECTIONS_PER_NODE;

	switch (currentValue.m_operation) {
	case Operation::Parameter:
	case Operation::Constant:
	{
		ImGui::PushItemWidth(node_width);
		ImGui::DragFloat(
			"##hidelabel", &currentValue.m_value, 0.01f);
		ImGui::PopItemWidth();

		ImNodes::BeginOutputAttribute(attribute_index + MAX_INPUTS);
		if (currentValue.m_operation == Operation::Parameter)
		{
			render_gradient(i, node_width);
		}

		ImNodes::EndOutputAttribute();
	}
	break;
	case Operation::Display:
	case Operation::Result:
	case Operation::Backwards:
	{
		ImNodes::BeginInputAttribute(attribute_index);
		ImGui::PushItemWidth(node_width);
		char text[128];
		sprintf(text, "%.3f", currentValue.m_value);
		ImGui::Text(text);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	break;
	case Operation::Add:
	case Operation::Subtract:
	case Operation::Divide:
	case Operation::Multiply:
	case Operation::Power:
	{
		for (int input = 0; input < 2; input++) {
			ImNodes::BeginInputAttribute(attribute_index + input);

			if (input == 0) {
				char text[128];
				sprintf(text, "%.3f", currentValue.m_value);

				const float label_width = ImGui::CalcTextSize(text).x;
				ImGui::Indent(node_width - label_width);
				ImGui::Text(text);
			}

			if (input == 1) {
				render_gradient(i, node_width);
			}

			ImNodes::EndInputAttribute();
		}

		ImNodes::BeginOutputAttribute(attribute_index + MAX_INPUTS);
		ImNodes::EndOutputAttribute();
	}
	break;
	case Operation::Tanh:
	case Operation::ReLU:
	case Operation::Sin:
	case Operation::Cos:
	case Operation::Sqrt:
	{
		ImNodes::BeginInputAttribute(attribute_index);
		{
			char text[128];
			sprintf(text, "%.3f", currentValue.m_value);

			const float label_width = ImGui::CalcTextSize(text).x;
			ImGui::Indent(node_width - label_width);
			ImGui::Text(text);
			ImNodes::EndInputAttribute();
		}

		ImNodes::BeginOutputAttribute(attribute_index + MAX_INPUTS);
		{
			render_gradient(i, node_width);
		}

		ImNodes::EndOutputAttribute();
	}
	break;
	case Operation::Function:
	{
		for (int input = 0; input < functions[function_node_data[i].m_function_id].m_num_inputs; input++) {
			ImNodes::BeginInputAttribute(attribute_index + input);
			ImGui::Text("%i", input);
			ImNodes::EndInputAttribute();
		}

		for (int output = 0; output < functions[function_node_data[i].m_function_id].m_num_outputs; output++) {
			ImNodes::BeginOutputAttribute(attribute_index + MAX_INPUTS + output);
			char text[128];
			sprintf(text, "%i", output);
			short function_id = function_node_data[i].m_function_id;
			float function_node_width = ImMax(ImGui::CalcTextSize(functions[function_id].m_name).x,node_width);
			const float label_width = ImGui::CalcTextSize(text).x;
			ImGui::Indent(function_node_width - label_width);
			ImGui::Text(text);
			ImNodes::EndOutputAttribute();
		}
	}
	break;
	case Operation::FunctionInput:
	{
		for (int i = 0; i < currentValue.m_variableNumConnections; i++) {
			ImNodes::BeginOutputAttribute(attribute_index + MAX_INPUTS + i);
			ImGui::Text("%i", i);
			ImNodes::EndOutputAttribute();
		}
	}
	break;
	case Operation::FunctionOutput:
	{
		for (int i = 0; i < currentValue.m_variableNumConnections; i++) {
			ImNodes::BeginInputAttribute(attribute_index + i);
			ImGui::Text("%i", i);
			ImNodes::EndOutputAttribute();
		}
	}
	break;
	case Operation::DataSource:
	{
		data_source.show_body(attribute_index + MAX_INPUTS);
	}	
	break;
	default:
		IM_ASSERT(0 && "Missing body for operation type");
		break;
	}
	if (i == current_backwards_node ||
		currentValue.m_operation == Operation::Parameter ||
		currentValue.m_operation == Operation::Constant  ||
		currentValue.m_operation == Operation::Display   ||
		currentValue.m_operation == Operation::Result    ||
		currentValue.m_operation == Operation::Backwards ) {
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
	}

	ImNodes::EndNode();
}

void ComputationGraph::show_connection(Connection connection) {
	Index start_parent = values[connection.start.node].m_parent;

	unsigned input_index = get_attribute_input_index(connection.end.node, connection.end.slot);

	if (true) {
		unsigned output_index = get_attribute_output_index(connection.start.node,
			connection.start.slot);

		ImNodes::Link(input_index,
			output_index,
			input_index);
	}
	else
	{
		for (int output_node = 0; output_node < function_node_data[start_parent].m_function_output_nodes.size(); output_node++) {
			if (function_node_data[start_parent].m_function_output_nodes[output_node].node == connection.start.node) {
				unsigned output_index = get_attribute_output_index(start_parent, output_node);

				ImNodes::Link(input_index,
					output_index,
					input_index);
			}
		}
	}
}

void ComputationGraph::show_connections(Index i, vector<Function>& functions) {
	Value& currentValue = values[i];

	//if (currentValue.m_parent == NULL_INDEX) {
		if (currentValue.m_operation != Operation::Function) {
			for (int input = 0; input < MAX_INPUTS; input++) {
				if (currentValue.m_inputs[input].node != NULL_INDEX) {
					Connection connection = Connection();
					connection.start = currentValue.m_inputs[input];
					connection.end.node = i;
					connection.end.slot = input;
						if (values[connection.start.node].m_parent == NULL_INDEX || values[connection.end.node].m_parent == NULL_INDEX ||
						values[connection.start.node].m_parent != values[connection.end.node].m_parent) {
						show_connection(connection);
					}
				}
			}	
		}
/*		else
		{
			if (currentValue.m_operation == Operation::Function)
			{
				for (int input = 0; input < function_node_data[currentValue.m_index].m_function_input_nodes.size(); input++) {
					Connection input_connection = function_node_data[currentValue.m_index].m_function_input_nodes[input];
					
					if (input_connection.end.node != NULL_INDEX) {
						Connection patched_connection(values[input_connection.end.node].m_inputs[input_connection.end.slot].node,
							values[input_connection.end.node].m_inputs[input_connection.end.slot].slot,
							i,
							input);
						if (patched_connection.start.node != NULL_INDEX) {
							show_connection(patched_connection);
						}
					}
				}
			}
		}
	}*/
}

void ComputationGraph::create_connection(Connection link) {
	Connection connection = Connection();

	Index patched_end_node  = link.end.node;
	Index patched_start_node = link.start.node;

	int   patched_end_slot  = link.end.slot;
	int   patched_start_slot = link.start.slot;
	//Patch function nodes through
	if (values[link.start.node].m_operation == Operation::Function) {
		patched_start_node = function_node_data[link.start.node].m_function_output_nodes[link.start.slot].node;
		patched_start_slot = function_node_data[link.start.node].m_function_output_nodes[link.start.slot].slot;
	}

	if (values[link.end.node].m_operation == Operation::Function) {
		patched_end_node = function_node_data[link.end.node].m_function_input_nodes[link.end.slot].node;
		patched_end_slot = function_node_data[link.end.node].m_function_input_nodes[link.end.slot].slot;
	}

	connection.start.node = patched_start_node;
	connection.start.slot = patched_start_slot;
	connection.end.node   = patched_end_node; 
	connection.end.slot   = patched_end_slot;

	apply_operation(EditOperation::add_connection(connection, patched_end_node));
}

void ComputationGraph::show(const int editor_id, bool* open, std::vector<Function>& functions, const char* name) {
	forwards(&data_source.data[data_source.current_data_point].x);

	zero_gradients();

	if (current_backwards_node != NULL_INDEX)
		backwards(&data_source.data[data_source.current_data_point].x);

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
						bool selected = minimap_location == locations[i];
						if (ImGui::MenuItem(names[i], NULL, &selected))
							minimap_location = locations[i];
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}
			ImNodes::BeginNodeEditor(editor_id);

			ImNodes::GetStyle().Flags &= ~ImNodesStyleFlags_GridLines;
			ImNodes::StyleColorsDark();
			for (int i = 0; i < next_free_index; i++) {
				if (used[i]) {
					Value& currentValue = values[i];

					if (currentValue.m_parent == NULL_INDEX) {
						show_node(i, functions);
					}
				}
			}

			for (int i = 0; i < next_free_index; i++) {
				if (used[i] && values[i].m_operation != Operation::Function) {
					show_connections(i, functions);
				}
			}

			ImNodes::MiniMap(0.2f, minimap_location);
			ImNodes::EndNodeEditor();

			if (nodes_to_select.size() > 0) {
				for (int i = 0; i < nodes_to_select.size(); i++) {
					if (used[nodes_to_select[i]] && values[nodes_to_select[i]].m_parent == NULL_INDEX) {
						ImNodes::SelectNode(nodes_to_select[i]);
					}
				}
				nodes_to_select.clear();
			}

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
				right_mouse_pressed_pos = ImGui::GetMousePos();
			}

			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
				ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
				ImGui::GetTime() - time_right_mouse_pressed < 0.3) {
				ImVec2 dist = (ImGui::GetMousePos() - right_mouse_pressed_pos);
				if (dist.x * dist.x + dist.y * dist.y < 10) {
					if (!nodes_selected && !node_hovered)
						ImGui::OpenPopup("none selected");
					else if ((node_hovered && !nodes_selected) || (node_hovered && !is_hovered_node_selected))
						ImGui::OpenPopup("node hovered");
					else if (nodes_selected)
						ImGui::OpenPopup("multiple nodes selected");
				}
			}

			if (ImGui::BeginPopup("none selected"))
			{
				const ImVec2 screen_click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();
				ImVec2 click_pos = ImNodes::ScreenSpaceToGridSpace(screen_click_pos);

				if (ImGui::MenuItem("Create Parameter Node")) {
					Value value = Value::make_value();
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}

				if (ImGui::MenuItem("Create Constant Node")) {
					Value value = Value::make_constant();
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Create Add Node")) {
					Value value = Value::make_add();
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create Subtract Node")) {
					Value value = Value::make_subtract();
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create Multiply Node")) {
					Value value = Value::make_multiply();
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create Divide Node")) {
					Value value = Value::make_divide();
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create Power Node")) {
					Value value = Value::make_power();
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create Tanh")) {
					Value value = Value::make_tanh();
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create ReLU")) {
					Value value = Value::make_value();
					value.m_operation = Operation::ReLU;
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create Sin")) {
					Value value = Value();
					value.m_operation = Operation::Sin;
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create Cos")) {
					Value value = Value();
					value.m_operation = Operation::Cos;
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create Sqrt")) {
					Value value = Value();
					value.m_operation = Operation::Sqrt;
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create Data Source Node")) {
					Value value = Value::make_data_source();
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create Result Node")) {
					Value value = Value::make_value();
					value.m_operation = Operation::Result;
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (ImGui::MenuItem("Create Backwards Node")) {
					Value value = Value::make_value();
					value.m_operation = Operation::Backwards;
					value.m_position = click_pos;
					EditOperation edit_operation = EditOperation::add_node(value);
					apply_operation(edit_operation);
				}
				if (functions.size() > 0) {
					ImGui::Separator();
					for (int i = 0; i < functions.size(); i++) {
						if (ImGui::MenuItem(functions[i].m_name)) {
							instantiate_function(i, click_pos, functions);
						}
					}
				}

				if (clip::has(clip::text_format())) {
					ImGui::Separator();
					if (ImGui::MenuItem("Paste")) {
						paste(click_pos);
					}
				}

				ImGui::EndPopup();
			}

			if (ImGui::IsKeyDown(ImGuiKey_A) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				Value value = Value::make_add();
				value.m_position = ImNodes::ScreenSpaceToGridSpace(ImGui::GetMousePos());
				EditOperation edit_operation = EditOperation::add_node(value);
				apply_operation(edit_operation);
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
					collapse_selected_nodes_to_new_function(click_pos, functions);
				}

				ImGui::EndPopup();
			}

			if (ImGui::BeginPopup("node hovered")) {
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
					apply_operation(op);
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

				bool is_first_input = firstNodeAttr < MAX_INPUTS;
				bool is_second_input = secondNodeAttr < MAX_INPUTS;

				if (is_first_input && !is_second_input) { 
					secondNodeAttr -= MAX_INPUTS;
					Connection connection;
					connection.end.node   = firstNode;
					connection.end.slot   = firstNodeAttr;
					connection.start.node = secondNode;
					connection.start.slot = secondNodeAttr;
					create_connection(connection);
				}
				else if (!is_first_input && is_second_input) {
					firstNodeAttr -= MAX_INPUTS;
					Connection connection;
					connection.start.node = firstNode;
					connection.start.slot = firstNodeAttr;
					connection.end.node   = secondNode;
					connection.end.slot   = secondNodeAttr;
					create_connection(connection);
				}
			}
		}
		ImGui::End();
	}

}