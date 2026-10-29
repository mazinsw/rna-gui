#ifndef _EDGE_H_
#define _EDGE_H_
#include "Node.h"
#include <string>

class Edge
{
private:
	Node* first;
	Node* second;
	double weight;
	bool selected;
	bool changed;
	Vector2 sz;
	std::string weightStr;
protected:

public:
	Edge(Node* first, Node* second, double weight);
	virtual ~Edge() {}

	inline void setFirst(Node* first)
	{
		this->first = first;
	}
	inline void setSecond(Node* second)
	{
		this->second = second;
	}
	inline void setWeight(double weight)
	{
		this->weight = weight;
		weightStr = Node::formatNumber(weight, 3);
	}

	inline Node* getFirst()
	{
		return this->first;
	}
	inline Node* getSecond()
	{
		return this->second;
	}
	inline double getWeight()
	{
		return this->weight;
	}

	/**
	 * Peso formatado com vírgula decimal, até 3 casas e sem zeros à direita
	 */
	inline const std::string& getWeightString()
	{
		return weightStr;
	}

	inline void setSelected(bool selected)
	{
		this->selected = selected;
	}

	inline bool isSelected()
	{
		return selected;
	}

	/**
	 * Marca o peso como alterado pelo último treinamento
	 */
	inline void setChanged(bool changed)
	{
		this->changed = changed;
	}

	inline bool isChanged()
	{
		return changed;
	}
	void draw();
	virtual bool IsIn(const Point& p);
};

#endif /* _EDGE_H_ */
