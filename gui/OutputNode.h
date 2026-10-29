#ifndef _OUTPUTNODE_H_
#define _OUTPUTNODE_H_
#include "Node.h"

class OutputNode: public Node
{
private:
	double target;
	Rectangle targetBox;
protected:

public:
	/**
	 * Diferença máxima entre o valor e o alvo para considerar a saída certa
	 */
	static constexpr double Tolerance = 0.05;
public:
	OutputNode();
	virtual Type getType() { return TypeOutput; }
	virtual bool IsIn(const Point& p);
	virtual void draw(bool running);
	virtual bool IsInValue(const Point& p);

	/**
	 * Valor desejado na saída, usado no treinamento
	 */
	inline double getTarget()
	{
		return target;
	}

	/**
	 * Alterna o alvo entre 0 e 1
	 */
	inline void toggleTarget()
	{
		target = target != 0 ? 0 : 1;
	}

	/**
	 * Indica se o valor calculado é igual ao alvo
	 */
	inline bool isOnTarget()
	{
		return fabs(target - getValue()) < Tolerance;
	}
};

#endif /* _OUTPUTNODE_H_ */
