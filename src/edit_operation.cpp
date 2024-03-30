#include "edit_operation.h"
#include "computation_graph.h"
void EditOperation::apply(ComputationGraph* context) {
	switch (m_type) {
	case EditOperationType::AddNode:
	{
		Index index = m_value.m_index;
		if (index == NULL_INDEX)
			index = context->get_new_value();
		context->values[index] = m_value;
		context->values[index].m_index = index;
		if (context->values[index].m_operation == Operation::Backwards) {
			context->current_backwards_node = index;
		}
		if (context->values[index].m_operation == Operation::Result) {
			context->current_result_node = index;
		}
		m_value.m_index = index;
		m_index = index;
	}
	break;
	case EditOperationType::RemoveNode:
		m_value = context->values[m_index];
		if (context->values[m_index].m_operation == Operation::Backwards) {
			context->current_backwards_node = NULL_INDEX;
		}
		if (context->values[m_index].m_operation == Operation::Result) {
			context->current_result_node = NULL_INDEX;
		}
		context->values[m_index] = Value();
		context->used[m_index] = false;
		break;
	case EditOperationType::AddLink:
		context->values[m_index].m_inputs[m_connection.end.slot] = m_connection.start;
		break;
	case EditOperationType::RemoveLink:
		context->values[m_index].m_inputs[m_connection.end.slot] = Socket();
		break;
	case EditOperationType::MoveNodes:
		context->values[m_index].m_position += m_pos_delta;
		break;
	default:
		break;
	}
}

void EditOperation::undo(ComputationGraph* context) {
	switch (m_type) {
	case EditOperationType::AddNode:
		context->values[m_index] = Value();
		break;
	case EditOperationType::RemoveNode:
	{
		Index index = m_value.m_index;
		context->values[index] = m_value;
		context->values[index].m_index = index;
		m_value.m_index = index;
		context->used[index] = true;
	}
	break;
	case EditOperationType::AddLink:
		context->values[m_index].m_inputs[m_connection.end.node] = Socket();
		break;
	case EditOperationType::RemoveLink:
		context->values[m_index].m_inputs[m_connection.end.slot] = m_connection.start;
		break;
	case EditOperationType::MoveNodes:
		context->values[m_index].m_position -= m_pos_delta;
		context->values[m_index].m_positionDirty = true;
		break;
	}
}

EditOperation EditOperation::add_node(const Value& value, const bool _final) {
	EditOperation op;
	op.m_type = EditOperationType::AddNode;
	op.m_value = value;
	op.m_final = _final;
	return op;
}

EditOperation EditOperation::remove_node(const Index index, const bool _final) {
	EditOperation op;
	op.m_type = EditOperationType::RemoveNode;
	op.m_index = index;
	op.m_final = _final;
	return op;
}

EditOperation EditOperation::add_connection(const Connection& connection, const bool _final) {
	EditOperation op;
	op.m_type = EditOperationType::AddLink;
	op.m_index = connection.end.node;
	op.m_connection = connection;
	op.m_final = _final;
	return op;
}

EditOperation EditOperation::remove_link(const Connection& connection,
	const Index index, const bool _final) {
	EditOperation op;
	op.m_type = EditOperationType::RemoveLink;
	op.m_index = index;
	op.m_connection = connection;
	op.m_final = _final;
	return op;
}

EditOperation EditOperation::move_node(const Index index, const ImVec2& delta, const bool _final) {
	EditOperation op;
	op.m_type = EditOperationType::MoveNodes;
	op.m_index = index;
	op.m_pos_delta = delta;
	op.m_final = _final;
	return op;
}
