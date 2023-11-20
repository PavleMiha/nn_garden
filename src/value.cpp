#include "value.h"

#define NUM_INPUTS 2

json Value::to_json() {
	json j;
	j["index"] = m_index;
	j["value"] = m_value;
	j["gradient"] = m_gradient;
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

void Value::from_json(json j) {
	m_index = j["index"];
	m_value = j["value"];
	m_gradient = j["gradient"];
	m_operation = j["operation"];
	for (int i = 0; i < j["inputs"].size(); i++) {
		int slot = j["inputs"][i]["input_slot"];
		m_inputs[slot].index = j["inputs"][i]["index"];
		m_inputs[slot].input_slot = j["inputs"][i]["input_slot"];
		m_inputs[slot].output_slot = j["inputs"][i]["output_slot"];
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
		if (input.index != NULL_INDEX) {
			vector<Index> m_parent1_descendants = values[input.index].get_topological_sorted_descendants_inner(visited, values);
			sorted_descendants.insert(sorted_descendants.end(), m_parent1_descendants.begin(), m_parent1_descendants.end());
		}
	}
	sorted_descendants.push_back(m_index);
	return sorted_descendants;
}

void Value::single_forwards(Value* values) {
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
		if (m_inputs[0].index != NULL_INDEX && m_inputs[1].index != NULL_INDEX)
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

void Value::single_backwards(Value* values) {
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

void Value::backwards(Value* values) {
	vector<Index> sorted_descendants = get_topological_sorted_descendants(values);

	for (auto& it : sorted_descendants) {
		values[it].m_gradient = 0.f;
	}

	m_gradient = 1.f;

	for (vector<Index>::reverse_iterator it = sorted_descendants.rbegin(); it != sorted_descendants.rend(); ++it) {
		values[*it].single_backwards(values);
	}
}