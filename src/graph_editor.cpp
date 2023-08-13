#include "imnodes.h"
#include <imgui.h>
#include <vector>
#include <unordered_set>
#define MAX_NODES 1024*1024
#define MAX_CONNECTIONS_PER_NODE 3
#define NULL_INDEX UINT_MAX
using std::vector;
using std::unordered_set;

enum class Operation {
	Add,
	Subtract,
	Multiply,
	Divide,
	None,
};

typedef unsigned Index;

class Value {
public:
	Index     m_index{ NULL_INDEX };
	float	  m_value{ 0.f };
	float	  m_gradient{ 0.f };
	Operation m_operation{ Operation::None };
	Index m_child1{ NULL_INDEX };
	Index m_child2{ NULL_INDEX };

	Value() {};
	Value(float value) : m_value(value) {};

	Value(float value, Operation operation, Index parent1, Index parent2) :
		m_value(value),
		m_operation(operation),
		m_child1(parent1),
		m_child2(parent2) {};

	friend Value operator+(Value& lhs, Value& rhs) {
		return Value(lhs.m_value + rhs.m_value, Operation::Add, lhs.m_index, rhs.m_index);
	}
	friend Value operator-(Value& lhs, Value& rhs) {
		return Value(lhs.m_value + rhs.m_value, Operation::Add, lhs.m_index, rhs.m_index);
	}
	friend Value operator*(Value& lhs, Value& rhs) {
		return Value(lhs.m_value * rhs.m_value, Operation::Multiply, lhs.m_index, rhs.m_index);
	}
	friend Value operator/(Value& lhs, Value& rhs) {
		return Value(lhs.m_value / rhs.m_value, Operation::Divide, lhs.m_index, rhs.m_index);
	}

	vector<Index> get_topological_sorted_descendants(Value* values) {
		unordered_set<Index> visited;
		return get_topological_sorted_descendants_inner(visited, values);
	}

	vector<Index> get_topological_sorted_descendants_inner(unordered_set<Index>& visited, Value* values) {
		vector<Index> sorted_descendants;
		visited.insert(m_index);
		if (m_child1 != NULL_INDEX && visited.find(m_child1) == visited.end())
		{
			vector<Index> m_parent1_descendants = values[m_child1].get_topological_sorted_descendants_inner(visited, values);
			sorted_descendants.insert(sorted_descendants.end(), m_parent1_descendants.begin(), m_parent1_descendants.end());
		}
		if (m_child2 != NULL_INDEX && visited.find(m_child2) == visited.end())
		{
			vector<Index> m_parent1_descendants = values[m_child2].get_topological_sorted_descendants_inner(visited, values);
			sorted_descendants.insert(sorted_descendants.end(), m_parent1_descendants.begin(), m_parent1_descendants.end());
		}
		sorted_descendants.push_back(m_index);
		return sorted_descendants;
	}

	void single_forwards(Value* values) {
		switch (m_operation) {
		case Operation::Add:
			if (m_child1 != NULL_INDEX && m_child2 != NULL_INDEX)
				m_value = values[m_child1].m_value + values[m_child2].m_value;
			break;
		case Operation::Multiply:
			if (m_child1 != NULL_INDEX && m_child2 != NULL_INDEX)
				m_value = values[m_child1].m_value * values[m_child2].m_value;
			break;
		case Operation::Subtract:
			if (m_child1 != NULL_INDEX && m_child2 != NULL_INDEX)
				m_value = values[m_child1].m_value - values[m_child2].m_value;
			break;
		case Operation::Divide:
			if (m_child1 != NULL_INDEX && m_child2 != NULL_INDEX)
				m_value = values[m_child1].m_value / values[m_child2].m_value;
			break;
		default:
			//assert((false && "Unknown operation"));
			break;
		}
	}

	void single_backwards(Value* values) {
		switch (m_operation) {
		case Operation::Add:
			values[m_child1].m_gradient += m_gradient;
			values[m_child2].m_gradient += m_gradient;
			break;
		case Operation::Multiply:
			values[m_child1].m_gradient += m_gradient * values[m_child2].m_value;
			values[m_child2].m_gradient += m_gradient * values[m_child1].m_value;
			break;
		case Operation::Subtract:
			values[m_child1].m_gradient += m_gradient;
			values[m_child2].m_gradient -= m_gradient;
			break;
		case Operation::Divide:
			values[m_child1].m_gradient += m_gradient / values[m_child2].m_value;
			values[m_child2].m_gradient -= m_gradient * values[m_child1].m_value / (values[m_child2].m_value * values[m_child2].m_value);
			break;
		default:
			//assert((false && "Unknown operation"));
			break;
		}
	}

	void backwards(Value* values) {
		vector<Index> sorted_descendants = get_topological_sorted_descendants(values);

		for (auto& it : sorted_descendants) {
			values[it].m_gradient = 0.f;
		}

		m_gradient = 1.f;

		for (vector<Index>::reverse_iterator it = sorted_descendants.rbegin(); it != sorted_descendants.rend(); ++it) {
			values[*it].single_backwards(values);
		}
	}

	static void forwards(Value* values, bool* alive) {
		//first we find all the nodes with no parents
		unordered_set <Index> checked;
		unordered_set <Index> has_parent;

		for (int i = 0; i < MAX_NODES; i++) {
			if (alive[i]) {
				checked.insert(i);
				if (values[i].m_child1 != NULL_INDEX) {
					has_parent.insert(values[i].m_child1);
				}
				if (values[i].m_child2 != NULL_INDEX) {
					has_parent.insert(values[i].m_child2);
				}
			}
		}

		unordered_set <Index> no_parent;

		for (auto& it : checked) {
			if (has_parent.find(it) == has_parent.end()) {
				no_parent.insert(it);
			}
		}

		unordered_set <Index> calculated;
		for (const auto& it : no_parent) {
			vector<Index> sorted_descendants = values[it].get_topological_sorted_descendants(values);

			for (const auto& descendant : sorted_descendants) {
				if (calculated.find(descendant) == calculated.end()) {
					values[descendant].single_forwards(values);
					calculated.insert(descendant);
				}
			}
		}

	}

	static void zero_gradients(Value* v, bool* used) {
		for (int i = 0; i < MAX_NODES; i++) {
			if (used[i])
				v[i].m_gradient = 0.f;
		}
	}

	Value& operator=(const Value& other) {
		m_value = other.m_value;
		m_gradient = other.m_gradient;
		m_operation = other.m_operation;
		m_child1 = other.m_child1;
		m_child2 = other.m_child2;
		return *this;
	}
};

bool  used[MAX_NODES];
Value values[MAX_NODES];
Index	 s_current_backwards_node = NULL_INDEX;
Index	 s_next_free_index = 0;

unsigned s_next_node_imgui_index = 0;
unsigned s_next_attribute_imgui_index = 0;
unsigned s_next_link_imgui_index = 0;

void create_value(Value** value, unsigned* index) {
	*index = s_next_free_index;
	*value = &values[s_next_free_index++];
}

Index get_new_value() {
	values[s_next_free_index].m_index = s_next_free_index;
	used[s_next_free_index] = true;
	return s_next_free_index++;
}

Value& get_value(Index index) {
	return values[index];
}

void start_test_net() {
	/*Index a = get_new_value();
	Index b = get_new_value();
	Index c = get_new_value();
	Index d = get_new_value();
	Index e = get_new_value();
	Index f = get_new_value();
	Index g = get_new_value();
	Index L = get_new_value();

	get_value(a).m_value = 2.0f;
	get_value(b).m_value = -3.0f;
	get_value(c).m_value = 10.f;
	get_value(e) = get_value(a) * get_value(b);
	get_value(d) = get_value(e) + get_value(c);
	get_value(f).m_value = -2.f;
	get_value(g) = get_value(b) * get_value(d);
	get_value(L) = get_value(g) * get_value(f);

	get_value(L).backwards(values);*/
}

int s_last_node_hovered = -1;

void show_graph_editor(bool* open) {
	Value::forwards(values, used);

	Value::zero_gradients(values, used);

	if (s_current_backwards_node != NULL_INDEX)
		get_value(s_current_backwards_node).backwards(values);

	auto flags = ImGuiWindowFlags_MenuBar;

	// The node editor window

	ImGui::Begin("Graph Editor", open, flags);

	ImNodes::BeginNodeEditor();


	for (int i = 0; i < MAX_NODES; i++) {
		if (used[i]) {

			const float node_width = 70.0f;

			if (i == s_current_backwards_node) {
				ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(191, 109, 11, 255));
				ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(194, 126, 45, 255));
				ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(204, 148, 81, 255));
			}

			if (values[i].m_operation == Operation::None)
			{
				ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(0x69, 0x0f, 0x62, 255));
				ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(0x8b, 0x26, 0x84, 255));
				ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(0x94, 0x2e, 0x8c, 255));
			}

			ImNodes::BeginNode(i);

			ImNodes::BeginNodeTitleBar();
			switch (values[i].m_operation) {
			case Operation::Add:
				ImGui::TextUnformatted("add");
				break;
			case Operation::Subtract:
				ImGui::TextUnformatted("sub");
				break;
			case Operation::Multiply:
				ImGui::TextUnformatted("mul");
				break;
			case Operation::Divide:
				ImGui::TextUnformatted("div");
				break;
			default:
				ImGui::TextUnformatted("value");
			}
			ImNodes::EndNodeTitleBar();
			unsigned attribute_index = i * MAX_CONNECTIONS_PER_NODE;

			{
				if (values[i].m_operation == Operation::None)
				{
					const float label_width = ImGui::CalcTextSize("value").x;

					ImGui::PushItemWidth(node_width - label_width);
					ImGui::DragFloat(
						"##hidelabel", &values[i].m_value, 0.01f);
					ImGui::PopItemWidth();
				}
				else
				{
					ImNodes::BeginInputAttribute(attribute_index);
					char text[128];
					sprintf(text, "%.1f", values[i].m_value);

					const float label_width = ImGui::CalcTextSize(text).x;
					ImGui::Indent(node_width - label_width);
					ImGui::Text(text, values[i].m_gradient);
					ImNodes::EndInputAttribute();
				}
			}

			if (values[i].m_operation == Operation::None)
			{
				char text[128];
				sprintf(text, "grad %.1f", values[i].m_gradient);

				const float label_width = ImGui::CalcTextSize(text).x;
				ImGui::Indent(node_width - label_width);
				ImGui::Text(text, values[i].m_gradient);
			}
			else
			{
				ImNodes::BeginInputAttribute(attribute_index + 1);

				char text[128];
				sprintf(text, "grad %.1f", values[i].m_gradient);

				const float label_width = ImGui::CalcTextSize(text).x;
				ImGui::Indent(node_width - label_width);
				ImGui::Text(text, values[i].m_gradient);
				ImNodes::EndInputAttribute();
				
			}
			ImNodes::BeginOutputAttribute(attribute_index + 2);
			ImNodes::EndOutputAttribute();

			if (i == s_current_backwards_node || values[i].m_operation == Operation::None) {
				ImNodes::PopColorStyle();
				ImNodes::PopColorStyle();
				ImNodes::PopColorStyle();
			}

			ImNodes::EndNode();


			if (values[i].m_child1 != NULL_INDEX) {
				ImNodes::Link(i * MAX_CONNECTIONS_PER_NODE, values[i].m_child1 * MAX_CONNECTIONS_PER_NODE + 2, attribute_index);
			}
			if (values[i].m_child2 != NULL_INDEX) {
				ImNodes::Link(i * MAX_CONNECTIONS_PER_NODE + 1, values[i].m_child2 * MAX_CONNECTIONS_PER_NODE + 2, attribute_index + 1);
			}
		}
	}

	ImNodes::EndNodeEditor();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
	const bool open_popup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
			ImGui::IsMouseReleased(ImGuiMouseButton_Right);

	if (open_popup)
	{
		bool hovered = ImNodes::IsNodeHovered(&s_last_node_hovered);
		if (hovered)
			ImGui::OpenPopup("node options");
		else
			ImGui::OpenPopup("add node");
	}

	if (ImGui::BeginPopup("add node"))
	{
		const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

		if (ImGui::MenuItem("Creat Value Node")) {
			Index node = get_new_value();
			values[node].m_operation = Operation::None;
			ImNodes::SetNodeScreenSpacePos(node, click_pos);
		}
		if (ImGui::MenuItem("Create Add Node")) {
			Index node = get_new_value();
			values[node].m_operation = Operation::Add;
			ImNodes::SetNodeScreenSpacePos(node, click_pos);
		}
		if (ImGui::MenuItem("Create Subtract Node")) {
			Index node = get_new_value();
			values[node].m_operation = Operation::Subtract;
			ImNodes::SetNodeScreenSpacePos(node, click_pos);
		}
		if (ImGui::MenuItem("Create Multiply Node")) {
			Index node = get_new_value();
			values[node].m_operation = Operation::Multiply;
			ImNodes::SetNodeScreenSpacePos(node, click_pos);
		}
		if (ImGui::MenuItem("Create Divide Node")) {
			Index node = get_new_value();
			values[node].m_operation = Operation::Divide;
			ImNodes::SetNodeScreenSpacePos(node, click_pos);
		}

		ImGui::EndPopup();

	}

	if (ImGui::BeginPopup("node options"))
	{
		const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

		if (ImGui::MenuItem("Backward Pass")) {
			s_current_backwards_node = s_last_node_hovered;
		}

		ImGui::EndPopup();
	}

	int start_attr, end_attr;
	if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
	{
		Index firstNode = start_attr/MAX_CONNECTIONS_PER_NODE;
		int firstNodeAttr = start_attr%MAX_CONNECTIONS_PER_NODE;

		Index secondNode = end_attr/MAX_CONNECTIONS_PER_NODE;
		int secondNodeAttr = end_attr%MAX_CONNECTIONS_PER_NODE;

		if (firstNodeAttr == 2 && secondNodeAttr == 0) {
			values[secondNode].m_child1 = firstNode;
		}
		else if (firstNodeAttr == 2 && secondNodeAttr == 1) {
			values[secondNode].m_child2 = firstNode;
		}
		else if (firstNodeAttr == 0 && secondNodeAttr == 2) {
			values[firstNode].m_child1 = secondNode;
		}
		else if (firstNodeAttr == 1 && secondNodeAttr == 2) {
			values[firstNode].m_child2 = secondNode;
		}
	}

	ImGui::PopStyleVar();


	ImGui::End();

}