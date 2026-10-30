#ifndef _OUTPUTNODE_H_
#define _OUTPUTNODE_H_
#include "Node.h"
#include "../training/Graph.h"

class OutputNode: public Node
{
private:
	double target;
	Rectangle targetBox;
protected:

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

	inline void setTarget(double target)
	{
		this->target = target;
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
		return fabs(target - getValue()) < Graph::Tolerance;
	}
};

#endif /* _OUTPUTNODE_H_ */
