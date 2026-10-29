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
	// 0 a 1, relativa à maior da rede
	float influence;
	float thickness;
	Rectangle labelBox;
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

	/**
	 * Influência na saída de 0 a 1, define a cor e a espessura da linha
	 */
	inline void setInfluence(float influence)
	{
		this->influence = influence;
	}

	/**
	 * Posição do rótulo do peso no último desenho
	 */
	inline Rectangle getLabelBox()
	{
		return labelBox;
	}

	void drawLine();

	/**
	 * Desenha o rótulo do peso, depois de todas as linhas para ficar por cima
	 */
	void drawLabel();
	virtual bool IsIn(const Point& p);
	bool IsInLabel(const Point& p);

	/**
	 * Do cinza ao vermelho, passando por âmbar e laranja
	 */
	static Color influenceColor(float influence);
};

#endif /* _EDGE_H_ */
