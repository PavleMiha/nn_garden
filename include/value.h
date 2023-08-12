#include <vector>
#include <set>
#include <unordered_set>
enum class Operation {
	Add,
	Subtract,
	Multiply,
	Divide,
	None,
};

using std::vector;
using std::set;
using std::unordered_set;

class Value {
public:
	float	  m_value{ 0.f };
	float	  m_gradient{ 0.f };
	Operation m_operation{ Operation::None };
	Value* m_child1{ nullptr };
	Value* m_child2{ nullptr };

	Value(float value) : m_value(value) {};

	Value(float value, Operation operation, Value* parent1, Value* parent2) :
		m_value(value),
		m_operation(operation),
		m_child1(parent1),
		m_child2(parent2) {};

	friend Value operator+(Value& lhs, Value& rhs) {
		return Value(lhs.m_value + rhs.m_value, Operation::Add, &lhs, &rhs);
	}

	friend Value operator*(Value& lhs, Value& rhs) {
		return Value(lhs.m_value * rhs.m_value, Operation::Multiply, &lhs, &rhs);
	}

	friend Value operator-(Value& lhs, Value& rhs) {
		return Value(lhs.m_value - rhs.m_value, Operation::Subtract, &lhs, &rhs);
	}

	friend Value operator/(Value& lhs, Value& rhs) {
		return Value(lhs.m_value / rhs.m_value, Operation::Divide, &lhs, &rhs);
	}

	vector<Value*> get_topological_sorted_descendants() {
		unordered_set<Value*> visited;
		return get_topological_sorted_descendants_inner(visited);
	}

	vector<Value*> get_topological_sorted_descendants_inner(unordered_set<Value*>& visited) {
		vector<Value*> sorted_descendants;
		visited.insert(this);
		if (m_child1 && visited.find(m_child1) == visited.end())
		{
			vector<Value*> m_parent1_descendants = m_child1->get_topological_sorted_descendants_inner(visited);
			sorted_descendants.insert(sorted_descendants.end(), m_parent1_descendants.begin(), m_parent1_descendants.end());
		}
		if (m_child2 && visited.find(m_child2) == visited.end())
		{
			vector<Value*> m_parent1_descendants = m_child2->get_topological_sorted_descendants_inner(visited);
			sorted_descendants.insert(sorted_descendants.end(), m_parent1_descendants.begin(), m_parent1_descendants.end());
		}
		sorted_descendants.push_back(this);
		return sorted_descendants;
	}

	void single_backwards() {
		switch (m_operation) {
		case Operation::Add:
			m_child1->m_gradient += m_gradient;
			m_child2->m_gradient += m_gradient;
			break;
		case Operation::Multiply:
			m_child1->m_gradient += m_gradient * m_child2->m_value;
			m_child2->m_gradient += m_gradient * m_child1->m_value;
			break;
		case Operation::Subtract:
			m_child1->m_gradient += m_gradient;
			m_child2->m_gradient -= m_gradient;
			break;
		case Operation::Divide:
			m_child1->m_gradient += m_gradient / m_child2->m_value;
			m_child2->m_gradient -= m_gradient * m_child1->m_value / (m_child2->m_value * m_child2->m_value);
			break;
		default:
			//assert((false && "Unknown operation"));
			break;
		}
	}

	void backwards() {
		vector<Value*> sorted_descendants = get_topological_sorted_descendants();

		for (auto& it : sorted_descendants) {
			it->m_gradient = 0.f;
		}

		m_gradient = 1.f;

		for (vector<Value*>::reverse_iterator it = sorted_descendants.rbegin(); it != sorted_descendants.rend(); ++it) {
			(*it)->single_backwards();
		}
	}
};
