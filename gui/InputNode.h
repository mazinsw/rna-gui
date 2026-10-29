#ifndef _INPUTNODE_H_
#define _INPUTNODE_H_
#include "Node.h"

class InputNode: public Node
{
private:
	bool bias;
public:
	InputNode();
	virtual Type getType() { return TypeInput; }
	virtual bool IsIn(const Point& p);
	virtual void draw(bool running);

	/**
	 * Alterna o valor da entrada entre 0 e 1, o bias fica sempre em 1
	 */
	inline void toggleValue()
	{
		if(!bias)
			setValue(getValue() != 0 ? 0 : 1);
	}

	/**
	 * Entrada de bias: valor fixo 1, alternativa ao bias de cada neurônio
	 */
	inline bool isBias()
	{
		return bias;
	}

	inline void setBias(bool bias)
	{
		this->bias = bias;
		setValue(bias ? 1 : 0);
	}
};

#endif /* _INPUTNODE_H_ */
