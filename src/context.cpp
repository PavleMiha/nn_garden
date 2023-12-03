#include "context.h"

void Context::clear() {
	//functions.clear();
	//main_graph.clear();
	//for (auto& f : function_graphs) {
	//	f.clear();
	//}
	//function_graphs.clear();
	//current_backwards_node = NULL_INDEX;
}

int Context::create_function(const json& json_data) {
	return 0;
}

void Context::show_function_list(bool* open) {
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

void Context::create_function_graph(int function_id) {
	ImNodes::CreateContext(function_id + 1);
	function_graphs[function_id].clear();
	function_graphs[function_id].from_json(functions[function_id].m_json, ImVec2());
	
	Index input_node_index = function_graphs[function_id].get_new_value();
	function_graphs[function_id].values[input_node_index].m_operation = Operation::FunctionInput;
	Index output_node_index = function_graphs[function_id].get_new_value();
	function_graphs[function_id].values[output_node_index].m_operation = Operation::FunctionOutput;

	int num_nodes = functions[function_id].m_json["nodes"].size();

	for (auto& node : functions[function_id].m_json["nodes"]) {
		for (int i = 0; i < node["inputs"].size(); i++) {
			if (node["inputs"][i]["index"] >= num_nodes) {
				Connection& input = function_graphs[function_id].values[node["index"]].m_inputs[i];
				input.index = input_node_index;
				input.input_slot = node["inputs"][i]["input_slot"];
				input.output_slot = node["inputs"][i]["index"]-num_nodes;
			}
		}
	}

	for (int i = 0; i < functions[function_id].m_json["unmatched_outputs"].size(); i++) {
		function_graphs[function_id].values[output_node_index].m_inputs[i].index = functions[function_id].m_json["unmatched_outputs"][i]["index"];
	}
}

void Context::show(bool* open) {
	main_graph.show(0, open, functions, "main graph");
	for (int function_id = 0; function_id < functions.size(); function_id++) {
		if (functions[function_id].m_is_open) {
			if (function_graphs.find(function_id) == function_graphs.end()) {
				create_function_graph(function_id);
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

void Context::save(const char* filename) {
	main_graph.save(filename);
}

void Context::load(const char* filename) {
	//ImNodes::BeginNodeEditor(0);
	main_graph.load(filename);
	//ImNodes::EndNodeEditor();
}

