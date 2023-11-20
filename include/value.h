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
	Function,
	FunctionInput,
	FunctionOutput,
	None,
};

typedef unsigned Index;

struct Connection {
public:
	Connection(Index _index, unsigned short _input_slot, unsigned short _output_slot) :
		index(_index), input_slot(_input_slot), output_slot(_output_slot) {};
	Connection() : index(NULL_INDEX), input_slot(0), output_slot(0) {};
	Index index{ NULL_INDEX };
	unsigned short input_slot{ 0 };
	unsigned short output_slot{ 0 };
};

class FunctionNodeData {
public:
	short				   m_function_id;
	vector<Connection>	   m_function_input_nodes;
	vector<Connection>	   m_function_output_nodes;
};

#define NUM_INPUTS 2

class Value {
public:
	Index			   m_index{ NULL_INDEX };
	float			   m_value{ 0.f };
	float			   m_gradient{ 0.f };
	Operation		   m_operation{ Operation::None };
	Connection		   m_inputs[NUM_INPUTS];
	Index			   m_parent{ NULL_INDEX };

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

	void single_forwards(Value* values);

	void single_backwards(Value* values);

	void backwards(Value* values);

	static Value make_value() {
		Value value;
		value.m_operation = Operation::None;
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
};