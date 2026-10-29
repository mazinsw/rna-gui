#ifndef _NEURONNODE_H_
#define _NEURONNODE_H_
#include "Node.h"

class NeuronNode: public Node
{
private:
	int functionType;
	double bias;
	bool biasChanged;
	bool biasSelected;
	Rectangle biasBox;
protected:
public:
	enum FunctionType
	{
		Linear = 0,
		Degrau,
		Bipolar,
		Logistic,
		Hiperbolic,
		FunctionCount
	};
public:
	NeuronNode();
	virtual Type getType() { return TypeNeuron; }
	virtual bool IsIn(const Point& p);
	virtual void draw(bool running);

	/**
	 * Indica se o ponto está sobre o rótulo do bias, acima do neurônio
	 */
	bool IsInBias(const Point& p);

	/**
	 * Posição do rótulo do bias no último desenho
	 */
	inline Rectangle getBiasBox()
	{
		return biasBox;
	}

	inline int getFunctionType()
	{
		return functionType;
	}

	inline void setFunctionType(int functionType)
	{
		this->functionType = functionType;
	}

	/**
	 * Somado à entrada da função de ativação, faz o papel do limiar negativo
	 */
	inline double getBias()
	{
		return bias;
	}

	inline void setBias(double bias)
	{
		this->bias = bias;
	}

	/**
	 * Marca o bias como alterado pelo último treinamento
	 */
	inline void setBiasChanged(bool changed)
	{
		biasChanged = changed;
	}

	inline void setBiasSelected(bool selected)
	{
		biasSelected = selected;
	}
};

#endif /* _NEURONNODE_H_ */
