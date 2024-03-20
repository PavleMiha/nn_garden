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
	Sin,
	Cos,
	Sqrt,
	Function,
	FunctionInput,
	FunctionOutput,
	Parameter,
	Constant,
	Result,
	DataSource,
};

typedef unsigned Index;

struct Socket {
	Socket(Index _node, unsigned short _slot) : node(_node), slot(_slot) {};
	Socket() : node(NULL_INDEX), slot(0) {};
	Index node;
	unsigned short slot;
};

struct Connection {
public:
	Connection(Index _start, unsigned short _start_slot, Index _end, unsigned short _end_slot) :
		start(_start, _start_slot), end(_end, _end_slot) {};
	Connection() : start(), end() {};
	
	Socket start;
	Socket end;
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
	char*	   m_name = nullptr;
	float	   m_value{ 0.f };
	ImVec2	   m_position;
	bool	   m_positionDirty{ true };
	float	   m_gradient{ 0.f };
	bool	   m_gradient_calculated{ false };
	Operation  m_operation{ Operation::Parameter };
	uint8_t	   m_variableNumConnections{ 0 };
	Socket	   m_inputs[MAX_INPUTS];//todo there's redundant information in here...
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

	float get_input_value(Value* values, int node, float* data_values);

	float get_value(int slot, float* data_values);

	void single_forwards(Value* values, float* data_values);

	void single_backwards(Value* values, float* data_values);

	void backwards(Value* values, float* data_values);

	static Value make_value() {
		Value value;
		value.m_operation = Operation::Parameter;
		return value;
	}

	static Value make_constant() {
		Value value;
		value.m_operation = Operation::Constant;
		return value;
	}

	static Value make_result() {
		Value value;
		value.m_operation = Operation::Result;
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