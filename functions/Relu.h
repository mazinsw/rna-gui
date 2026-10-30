#ifndef _RELU_H_
#define _RELU_H_
#include "../core/Function.h"

class ReluFunction: public Function
{
private:

protected:

public:
	/**
	 * Unidade linear retificada: o próprio x quando positivo, senão 0
	 */
	virtual double execute(double x)
	{
		return x > 0 ? x : 0;
	}

	virtual const char* getName()
	{
		static char name[] = "relu";
		return name;
	}
};

#endif /* _RELU_H_ */
