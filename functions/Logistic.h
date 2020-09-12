#ifndef _LOGISTIC_H_
#define _LOGISTIC_H_
#include "../core/Function.h"
#include <math.h>

class LogisticFunction: public Function
{
private:
	double beta;
protected:
	
public:
	/**
	 * Constroi a fun��o log�stica e informa a contante real associada ao 
	 * n�vel de inclina��o da fun��o
	 */
	LogisticFunction(double beta)
	{
		this->beta = beta;
	}
	
	virtual double execute(double x)
	{
		return 1.0 / (1.0 + pow(M_E, - beta * x));
	}
	
	virtual const char* getName()
	{
		static char name[] = "logistica";
		return name;
	}
};

#endif /* _LOGISTIC_H_ */