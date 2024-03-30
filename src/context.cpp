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
			const char* items[] = { "flat.csv", "wavy.csv", "donut.csv", "spiral.csv" };

			static int item_current_idx = 0;                    // Here our selection data is an index.
			const char* combo_label = items[item_current_idx];

			ImGui::SetNextItemWidth(200);
			if (ImGui::BeginCombo("##combo", combo_label)) {
				for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
					const bool is_selected = (item_current_idx == n);
					if (ImGui::Selectable(items[n], is_selected)) {
						item_current_idx = n;

						main_graph.data_source.load(items[n]);
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

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
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_PAINT_BRUSH)) {
				main_graph.data_source.update_image(&main_graph);
			}

			double current_time = ImGui::GetTime();

			double delta = tps_last_time - current_time;
			ImGui::Text("Steps: %i", training_steps);
			ImGui::Text("tps: %.3f", (delta / (double)training_steps_this_interval) * 1000.0f);
			ImGui::SameLine();
			ImGui::Text("Average Error: %.3f", current_average_error);
			tps_last_time = current_time;
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
		}
		ImGui::End();
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
	if (m_training && main_graph.current_backwards_node != NULL_INDEX) {
		double startTime = glfwGetTime();
		Index data_source_index = NULL_INDEX;
		for (int i = 0; i < MAX_NODES; i++) {
			if (main_graph.values[i].m_operation == Operation::DataSource) {
				data_source_index = i;
				break;
			}
		}

		training_steps_this_interval = 0;

		while (glfwGetTime() - startTime < (1.0f/30.f)) {
			main_graph.data_source.set_current_data_point(
				rand() % main_graph.data_source.data.size());
			main_graph.do_stochastic_gradient_descent(learning_rate);
			current_average_error = main_graph.values[main_graph.current_backwards_node].m_value * 0.001f + current_average_error * 0.999f;
			
			training_steps++;
			training_steps_this_interval++;
		}
		main_graph.data_source.update_image(&main_graph);
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
	json save_json = json();
	json main_graph_json = main_graph.to_json();

	save_json["main_graph"] = main_graph_json;

	json functions_json = json();

	for (int i = 0; i < functions.size(); i++) {
		json function_json = json();
		function_json["json"] = functions[i].m_json;
		function_json["num_inputs"] = functions[i].m_num_inputs;
		function_json["num_outputs"] = functions[i].m_num_outputs;
		function_json["name"] = functions[i].m_name;
		functions_json.push_back(function_json);
	}
	save_json["functions"] = functions_json;

	FILE* save_file = fopen(filename, "w");
	std::string dump = save_json.dump(4);
	fwrite(dump.c_str(), 1, dump.size(), save_file);
	fclose(save_file);
}

void Context::load(const char* filename) {
	//ImNodes::BeginNodeEditor(0);
	json content = json();
	FILE* save_file = fopen(filename, "rb");

	if (save_file) {
		fseek(save_file, 0, SEEK_END);
		long size = ftell(save_file);
		if (size != -1L) {
			fseek(save_file, 0, SEEK_SET);
			char* buffer = new char[size + 1];
			fread(buffer, 1, (size_t)size, save_file);
			buffer[size] = 0;
			fclose(save_file);
			content = json::parse(buffer);
			delete[] buffer;
			clear();
		}
		else {
			return;
		}
	}
	else {
		return;
	}

	main_graph.from_json(content["main_graph"], ImVec2());

	for (int i = 0; i < content["functions"].size(); i++) {
		Function function;
		function.m_json = content["functions"][i]["json"];
		function.m_is_open = false;
		function.m_num_inputs = content["functions"][i]["num_inputs"];
		function.m_num_outputs = content["functions"][i]["num_outputs"];
		std::string name = content["functions"][i]["name"];
		strcpy(function.m_name, name.c_str());
		functions.push_back(function);
	}
}

