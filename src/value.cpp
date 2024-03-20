#include "value.h"


json Value::to_json() {
	json j;
	j["index"] = m_index;
	j["value"] = m_value;
	j["gradient"] = m_gradient;
	j["operation"] = m_operation;
	if (m_name != nullptr) {
		j["name"] = m_name;
	}
	for (int i = 0; i < MAX_INPUTS; i++) {
		if (m_inputs[i].node != NULL_INDEX) {
			j["inputs"][i]["node"] = m_inputs[i].node;
			j["inputs"][i]["slot"] = m_inputs[i].slot;
		}
	}
	return j;
}

void Value::from_json(json j) {
	m_index = j["index"];
	m_value = j["value"];
	if (j.contains("gradient") && !j["gradient"].is_null())
		m_gradient = j["gradient"];
	m_operation = j["operation"];
	if (j.contains("name")) {
		m_name = (char*)malloc(128);
		std::string name;
		j["name"].get_to(name);
		strcpy(m_name, name.c_str());
	}
	for (int i = 0; i < j["inputs"].size(); i++) {
		if (j["inputs"][i].is_null() || j["inputs"][i]["node"] == NULL_INDEX) {
			continue;
		}
		m_inputs[i].node = j["inputs"][i]["node"];
		m_inputs[i].slot = j["inputs"][i]["slot"];
	}
}

void Value::set_operation(Operation operation) {
	m_operation = operation;
}

vector<Index> Value::get_topological_sorted_descendants(Value* values) {
	unordered_set<Index> visited;
	return get_topological_sorted_descendants_inner(visited, values);
}

vector<Index> Value::get_topological_sorted_descendants_inner(unordered_set<Index>& visited, Value* values) {
	vector<Index> sorted_descendants;
	visited.insert(m_index);
	for (const auto& input : m_inputs) {
		if (input.node != NULL_INDEX) {
			vector<Index> m_parent1_descendants = values[input.node].get_topological_sorted_descendants_inner(visited, values);
			sorted_descendants.insert(sorted_descendants.end(), m_parent1_descendants.begin(), m_parent1_descendants.end());
		}
	}
	sorted_descendants.push_back(m_index);
	return sorted_descendants;
}

float Value::get_input_value(Value* values, int input, float* data_values) {
	if (m_inputs[input].node == NULL_INDEX)
		return 0;
	else
		return values[m_inputs[input].node].get_value(m_inputs[input].slot, data_values);
}

float Value::get_value(int slot, float* data_values) {
	if (m_operation == Operation::DataSource) {
		return data_values[slot];
	}
	else {
		return m_value;
	}
}

void Value::single_forwards(Value* values, float* data_values) {

	switch (m_operation) {
	case Operation::Add:
		m_value = get_input_value(values, 0, data_values) + get_input_value(values, 1, data_values);
		break;
	case Operation::Multiply:
		m_value = get_input_value(values, 0, data_values) * get_input_value(values, 1, data_values);
		break;
	case Operation::Subtract:
		m_value = get_input_value(values, 0, data_values) - get_input_value(values, 1, data_values);
		break;
	case Operation::Divide:
		m_value = get_input_value(values, 0, data_values) / get_input_value(values, 1, data_values);
		break;
	case Operation::Power:
		m_value = std::powf(get_input_value(values, 0, data_values), get_input_value(values, 1, data_values));
		break;
	case Operation::Tanh:
		m_value = std::tanh(get_input_value(values, 0, data_values));
		break;
	case Operation::Sin:
		m_value = std::sin(get_input_value(values, 0, data_values));
		break;
	case Operation::Cos:
		m_value = std::cos(get_input_value(values, 0, data_values));
		break;
	case Operation::Sqrt:
		m_value = std::sqrt(get_input_value(values, 0, data_values));
		break;
	case Operation::Result:
		m_value = get_input_value(values, 0, data_values);
		break;
	default:
		//assert((false && "Unknown operation"));
		break;
	}
}

void Value::single_backwards(Value* values, float* data_values) {
	switch (m_operation) {
	case Operation::Add:
		if (m_inputs[0].node != NULL_INDEX)
			values[m_inputs[0].node].m_gradient += m_gradient;
		if (m_inputs[1].node != NULL_INDEX)
			values[m_inputs[1].node].m_gradient += m_gradient;
		break;
	case Operation::Multiply:
		if (m_inputs[0].node != NULL_INDEX) {
			values[m_inputs[0].node].m_gradient += m_gradient * get_input_value(values, 1, data_values);
		}
		if (m_inputs[1].node != NULL_INDEX) {
			values[m_inputs[1].node].m_gradient += m_gradient * get_input_value(values, 0, data_values);
		}
		break;
	case Operation::Subtract:
		if (m_inputs[0].node != NULL_INDEX) {
			values[m_inputs[0].node].m_gradient += m_gradient;
		}
		if (m_inputs[1].node != NULL_INDEX) {
			values[m_inputs[1].node].m_gradient -= m_gradient;
		}
		break;
	case Operation::Divide:
		if (m_inputs[0].node != NULL_INDEX && get_input_value(values, 1, data_values) != 0) {
			values[m_inputs[0].node].m_gradient += m_gradient / values[m_inputs[1].node].m_value;
		}
		if (m_inputs[1].node != NULL_INDEX) {
			values[m_inputs[1].node].m_gradient -= m_gradient * get_input_value(values, 0, data_values) /
				(values[m_inputs[1].node].m_value * values[m_inputs[1].node].m_value);
		}
		break;
	case Operation::Power:
		if (m_inputs[0].node != NULL_INDEX) {
			values[m_inputs[0].node].m_gradient += m_gradient * values[m_inputs[1].node].m_value *
				pow(values[m_inputs[0].node].m_value, values[m_inputs[1].node].m_value - 1);
		}
		if (m_inputs[1].node != NULL_INDEX) {
			values[m_inputs[1].node].m_gradient += m_gradient * pow(get_input_value(values, 0, data_values), 
				values[m_inputs[1].node].m_value) * log(get_input_value(values, 0, data_values));
		}
		break;
	case Operation::Tanh:
		if (m_inputs[0].node != NULL_INDEX) {
			values[m_inputs[0].node].m_gradient += m_gradient * (1.0f - std::powf(m_value, 2));
		}
		break;
	case Operation::Sin:
		if (m_inputs[0].node != NULL_INDEX) {
			values[m_inputs[0].node].m_gradient += m_gradient * std::cos(m_value);
		}
		break;
	case Operation::Cos:
		if (m_inputs[0].node != NULL_INDEX) {
			values[m_inputs[0].node].m_gradient += m_gradient * std::sin(m_value);
		}
		break;
	case Operation::Sqrt:
		if (m_inputs[0].node != NULL_INDEX) {
			values[m_inputs[0].node].m_gradient += m_gradient * (1.0f / (2.0f * std::sqrt(m_value)));
		}
		break;
	case Operation::Result:
		if (m_inputs[0].node != NULL_INDEX) {
			values[m_inputs[0].node].m_gradient += m_gradient;
		}
		break;

	default:
		//assert((false && "Unknown operation"));
		break;
	}
}

void Value::backwards(Value* values, float* data_values) {
	vector<Index> sorted_descendants = get_topological_sorted_descendants(values);

	for (auto& it : sorted_descendants) {
		values[it].m_gradient = 0.f;
		values[it].m_gradient_calculated = true;
	}

	m_gradient = 1.f;
	m_gradient_calculated = true;

	for (vector<Index>::reverse_iterator it = sorted_descendants.rbegin(); it != sorted_descendants.rend(); ++it) {
		values[*it].single_backwards(values, data_values);
	}
}