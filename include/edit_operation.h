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
	EditOperationType m_type		  = EditOperationType::AddNode;
	Value			  m_value		  = Value();
	Index			  m_index		  = NULL_INDEX;
	Index			  m_previousIndex = NULL_INDEX;
	Connection		  m_connection	  = Connection();
	ImVec2			  m_pos_delta	  = ImVec2();
	bool			  m_final		  = false;

	void EditOperation::apply(ComputationGraph* context);
	void EditOperation::undo(ComputationGraph* context);
	static EditOperation add_node(const Value& value, const bool _final = true);
	static EditOperation remove_node(const Index index, const bool _final = true);
	static EditOperation add_connection(const Connection& connection, const bool _final = true);
	static EditOperation remove_link(const Connection& connection, const Index index, const bool _final = true);
	static EditOperation move_node(const Index index, const ImVec2& delta, const bool _final = true);
};
