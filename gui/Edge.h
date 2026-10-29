#ifndef _EDGE_H_
#define _EDGE_H_
#include "Node.h"
#include <algorithm>
#include <stdio.h>
#include <string>

class Edge
{
private:
	Node* first;
	Node* second;
	double weight;
	bool selected;
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
		char str[64];
		this->weight = weight;
		snprintf(str, sizeof(str), "%lf", getWeight());
		weightStr = trimZeroes(str);
		std::replace(weightStr.begin(), weightStr.end(), '.', ',');
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
	 * Peso formatado com vírgula decimal e sem zeros à direita
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
	void draw();
	virtual bool IsIn(const Point& p);
	static std::string trimZeroes(const std::string& s);
};

#endif /* _EDGE_H_ */
