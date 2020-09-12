#ifndef _HIPERBOLIC_H_
#define _HIPERBOLIC_H_
#include "../core/Function.h"
#include <math.h>

class HiperbolicFunction: public Function
{
private:
	double beta;
protected:
	
public:
	/**
	 * Constroi a fun��o hiperb�lica e informa a contante real associada ao 
	 * n�vel de inclina��o da fun��o
	 */
	HiperbolicFunction(double beta)
	{
		this->beta = beta;
	}
	
	virtual double execute(double x)
	{
		double ep = pow(M_E, - beta * x);
		return (1.0 - ep) / (1.0 + ep);
	}
	
	virtual const char* getName()
	{
		static char name[] = "hiperbolica";
		return name;
	}
};

#endif /* _HIPERBOLIC_H_ */