#include "context.h"
#include "icons_font_awesome.h"

extern ImFont* imguiIconFont;

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

void Context::show_training_menu(bool* open) {
	if (*open) {
		//ImGui::SetNextWindowSize(ImVec2(200, 100));
		if (ImGui::Begin("Training Menu", open)) {
			if (ImGui::Button(ICON_FA_REFRESH)) {
				main_graph.randomize_parameters();
			}
			if (ImGui::BeginItemTooltip()) {
				ImGui::Text("Randomize Parameters");
				ImGui::EndTooltip();
			}
			ImGui::SameLine();
			ImGui::Text("Learning Rate:");
			ImGui::SameLine();
			ImGui::PushItemWidth(120);
			ImGui::InputFloat("##learning_rate", &learning_rate, 0.01f);
			ImGui::PopItemWidth();

			ImGui::SameLine();
				
			if (ImGui::Button(ICON_FA_ARROW_LEFT))
			{
				main_graph.do_stochastic_gradient_descent(learning_rate);
			}

			if (ImGui::BeginItemTooltip()) {
				ImGui::Text("Do one step of learning");
				ImGui::EndTooltip();
			}

			ImGui::SameLine();
			if (m_training) {
				if (ImGui::Button(ICON_FA_PAUSE)) {
					m_training = false;
				}
			}
			else if (ImGui::Button(ICON_FA_PLAY)) {
				m_training = true;
			}
			ImGui::Text("Steps: %i", training_steps);
			ImGui::SameLine();
			ImGui::Text("Average Error: %.3f", current_average_error);
		}
		ImGui::End();
	}
}

void Context::show_function_list(bool* open) {
	if (*open) {
		if (ImGui::Begin("Function List", open)) {
			for (int i = 0; i < functions.size(); i++) {
				char label[256] = {};
				sprintf(label, "##name%i", i);
				ImGui::InputText(label, functions[i].m_name, 128);
				ImGui::SameLine();
				ImGui::MenuItem("Open", "", &functions[i].m_is_open);
			}
			ImGui::End();
		}
	}
}

void Context::create_function_graph(int function_id) {
	ImNodes::CreateContext(function_id + 1);

	ComputationGraph& function_graph = function_graphs[function_id];
	Function& function = functions[function_id];
	function_graph.clear();
	function_graph.from_json(function.m_json, ImVec2());
	
	Index input_node_index = function_graph.get_new_value();
	function_graph.values[input_node_index].m_operation = Operation::FunctionInput;

	Index output_node_index = function_graph.get_new_value();
	function_graph.values[output_node_index].m_operation = Operation::FunctionOutput;

	int num_nodes = function.m_json["nodes"].size();

	function_graph.values[input_node_index].m_variableNumConnections = 0;
	for (auto& node : function.m_json["nodes"]) {
		for (int i = 0; i < node["inputs"].size(); i++) {
			if (node["inputs"][i]["index"] >= num_nodes) {
				Socket& input = function_graph.values[node["index"]].m_inputs[i];
				input.node = input_node_index;
				input.slot = node["inputs"][i]["index"]-num_nodes;
				function_graph.values[input_node_index].m_variableNumConnections++;
			}
		}
	}

	function_graph.values[output_node_index].m_variableNumConnections = 0;

	for (int i = 0; i < function.m_json["unmatched_outputs"].size(); i++) {
		function_graph.values[output_node_index].m_inputs[i].node = functions[function_id].m_json["unmatched_outputs"][i]["index"];
		function_graph.values[output_node_index].m_variableNumConnections++;
	}
}

void Context::show(bool* open) {
	if (m_training) {
		double startTime = glfwGetTime();
		Index data_source_index = NULL_INDEX;
		for (int i = 0; i < MAX_NODES; i++) {
			if (main_graph.values[i].m_operation == Operation::DataSource) {
				data_source_index = i;
				break;
			}
		}

		while (glfwGetTime() - startTime < (1.0f/30.f)) {
			main_graph.data_source.set_current_data_point(
				(main_graph.data_source.current_data_point + 1)%main_graph.data_source.data.size());
			main_graph.do_stochastic_gradient_descent(learning_rate);
			current_average_error = main_graph.values[main_graph.current_backwards_node].m_value * 0.01f + current_average_error * 0.99f;
			training_steps++;
		}
	}
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

