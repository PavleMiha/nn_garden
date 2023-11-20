#pragma once
#include "value.h"

class ComputationGraph;
class Context;

enum class EditOperationType {
	AddNode,
	RemoveNode,
	AddLink,
	RemoveLink,
	MoveNodes,
	SetBackwardsNode
};

class EditOperation {
public:
	EditOperationType m_type;
	Value			  m_value;
	Index			  m_index;
	Index			  m_previousIndex;
	Connection		  m_connection;
	ImVec2			  m_position;
	ImVec2			  m_pos_delta;
	bool			  m_final;

	void EditOperation::apply(ComputationGraph* context);
	void EditOperation::undo(ComputationGraph* context);
	static EditOperation add_node(const Value& value, const ImVec2& pos, const bool _final = true);
	static EditOperation remove_node(const Index index, const bool _final = true);
	static EditOperation add_link(const Connection& connection, const Index index, const bool _final = true);
	static EditOperation remove_link(const Connection& connection, const Index index, const bool _final = true);
	static EditOperation move_node(const Index index, const ImVec2& delta, const bool _final = true);
};
