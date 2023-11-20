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

void Context::show(bool* open) {
	main_graph.show(0, open, functions, "main graph");
	for (int function_id = 0; function_id < functions.size(); function_id++) {
		if (functions[function_id].m_is_open) {
			if (function_graphs.find(function_id) == function_graphs.end()) {
				ImNodes::CreateContext(function_id + 1);
				function_graphs[function_id].clear();
				function_graphs[function_id].from_json(functions[function_id].m_json, ImVec2());
				EditOperation op = EditOperation::add_node(Value::make_function_input(), ImVec2());
				function_graphs[function_id].apply_operation(op);
				op = EditOperation::add_node(Value::make_function_output(), ImVec2());
				function_graphs[function_id].apply_operation(op);
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

