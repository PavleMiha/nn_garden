#include "value.h"


json Value::to_json() {
	json j;
	j["index"] = m_index;
	j["value"] = m_value;
	j["gradient"] = m_gradient;
	j["operation"] = m_operation;
	for (int i = 0; i < MAX_INPUTS; i++) {
		if (m_inputs[i].end != NULL_INDEX) {
			j["inputs"][i]["end"]		 = m_inputs[i].end;
			j["inputs"][i]["end_slot"]   = m_inputs[i].end_slot;
			j["inputs"][i]["start"]      = m_inputs[i].start;
			j["inputs"][i]["start_slot"] = m_inputs[i].start_slot;
		}
	}
	return j;
}

void Value::from_json(json j) {
	m_index = j["index"];
	m_value = j["value"];
	m_gradient = j["gradient"];
	m_operation = j["operation"];
	for (int i = 0; i < j["inputs"].size(); i++) {
		int slot = j["inputs"][i]["end_slot"];
		m_inputs[slot].end		  = j["inputs"][i]["end"];
		m_inputs[slot].end_slot   = j["inputs"][i]["end_slot"];
		m_inputs[slot].start	  = j["inputs"][i]["start"];
		m_inputs[slot].start_slot = j["inputs"][i]["start_slot"];
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
		if (input.start != NULL_INDEX) {
			vector<Index> m_parent1_descendants = values[input.start].get_topological_sorted_descendants_inner(visited, values);
			sorted_descendants.insert(sorted_descendants.end(), m_parent1_descendants.begin(), m_parent1_descendants.end());
		}
	}
	sorted_descendants.push_back(m_index);
	return sorted_descendants;
}

float Value::get_input_value(Value* values, int input, float* data_values) {
	if (m_inputs[input].start == NULL_INDEX)
		return 0;
	else
		return values[m_inputs[input].start].get_value(m_inputs[input].start_slot, data_values);
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
	default:
		//assert((false && "Unknown operation"));
		break;
	}
}

void Value::single_backwards(Value* values, float* data_values) {
	switch (m_operation) {
	case Operation::Add:
		if (m_inputs[0].start != NULL_INDEX)
			values[m_inputs[0].start].m_gradient += m_gradient;
		if (m_inputs[1].start != NULL_INDEX)
			values[m_inputs[1].start].m_gradient += m_gradient;
		break;
	case Operation::Multiply:
		if (m_inputs[0].start != NULL_INDEX && m_inputs[1].start != NULL_INDEX) {
			values[m_inputs[0].start].m_gradient += m_gradient * values[m_inputs[1].start].m_value;
			values[m_inputs[1].start].m_gradient += m_gradient * values[m_inputs[0].start].m_value;
		}
		break;
	case Operation::Subtract:
		if (m_inputs[0].start != NULL_INDEX) {
			values[m_inputs[0].start].m_gradient += m_gradient;
		}
		if (m_inputs[1].start != NULL_INDEX) {
			values[m_inputs[1].start].m_gradient -= m_gradient;
		}
		break;
	case Operation::Divide:
		if (m_inputs[0].start != NULL_INDEX && m_inputs[1].start != NULL_INDEX) {
			values[m_inputs[0].start].m_gradient += m_gradient / values[m_inputs[1].start].m_value;
			values[m_inputs[1].start].m_gradient -= m_gradient * values[m_inputs[0].start].m_value /
				(values[m_inputs[1].start].m_value * values[m_inputs[1].start].m_value);
		}
		break;
	case Operation::Power:
		if (m_inputs[0].start != NULL_INDEX && m_inputs[1].start != NULL_INDEX) {
			values[m_inputs[0].start].m_gradient += m_gradient * values[m_inputs[1].start].m_value *
				pow(values[m_inputs[0].start].m_value, values[m_inputs[1].start].m_value - 1);
			values[m_inputs[1].start].m_gradient += m_gradient * pow(values[m_inputs[0].start].m_value,
				values[m_inputs[1].start].m_value) * log(values[m_inputs[0].start].m_value);
		}
		break;
	case Operation::Tanh:
		if (m_inputs[0].start != NULL_INDEX) {
			values[m_inputs[0].start].m_gradient += m_gradient * (1.0f - std::powf(m_value, 2));
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
	}

	m_gradient = 1.f;

	for (vector<Index>::reverse_iterator it = sorted_descendants.rbegin(); it != sorted_descendants.rend(); ++it) {
		values[*it].single_backwards(values, data_values);
	}
}