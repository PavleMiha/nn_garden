#pragma once

#include "imnodes.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <unordered_set>
#include <iostream>
#include "clip.h"
#include "json.hpp"

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
	Tanh,
	Function,
	FunctionInput,
	FunctionOutput,
	Value,
	DataSource,
};

typedef unsigned Index;

struct Connection {
public:
	Connection(Index _start, unsigned short _start_slot, Index _end, unsigned short _end_slot) :
		start(_start), start_slot(_start_slot), end(_end), end_slot(_end_slot) {};
	Connection() : start(NULL_INDEX), start_slot(0), end(NULL_INDEX), end_slot(0) {};

	Index start				  { NULL_INDEX };
	Index end			      { NULL_INDEX };
	unsigned short start_slot { 0 };
	unsigned short end_slot   { 0 };
};

class FunctionNodeData {
public:
	short				   m_function_id;
	vector<Connection>	   m_function_input_nodes;
	vector<Connection>	   m_function_output_nodes;
};

#define MAX_INPUTS 16

class Value {
public:
	Index	   m_index{ NULL_INDEX };
	float	   m_value{ 0.f };
	ImVec2	   m_position;
	bool	   m_positionDirty{ true };
	float	   m_gradient{ 0.f };
	Operation  m_operation{ Operation::Value };
	uint8_t	   m_variableNumConnections{ 0 };
	Connection m_inputs[MAX_INPUTS];//todo there's redundant information in here...
	Index	   m_parent{ NULL_INDEX };

	json to_json();

	void from_json(json j);

	Value() {};
	Value(float value) : m_value(value) {};
	Value(float value, Operation operation, Index parent1, Index parent2) :
		m_value(value),
		m_operation(operation),
		m_inputs()
	{};

	void set_operation(Operation operation);

	vector<Index> get_topological_sorted_descendants(Value* values);

	vector<Index> get_topological_sorted_descendants_inner(unordered_set<Index>& visited, Value* values);

	float get_value(int slot, float* data_values);

	void single_forwards(Value* values, float* data_values);

	void single_backwards(Value* values, float* data_values);

	void backwards(Value* values, float* data_values);

	static Value make_value() {
		Value value;
		value.m_operation = Operation::Value;
		return value;
	}

	static Value make_function_input() {
		Value value;
		value.m_operation = Operation::FunctionInput;
		return value;
	}

	static Value make_function_output() {
		Value value;
		value.m_operation = Operation::FunctionOutput;
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

	static Value make_tanh() {
		Value value;
		value.m_operation = Operation::Tanh;
		return value;
	}

	static Value make_data_source() {
		Value value;
		value.m_operation = Operation::DataSource;
		return value;
	}
};