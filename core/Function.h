#ifndef _FUNCTION_H_
#define _FUNCTION_H_

class Function
{
private:
	
protected:
	
public:
	/**
	 * Executa a fun��o e retorna o valor calculado usando a 
	 * vari�vel x
	 */
	virtual double execute(double x) = 0;
	virtual const char* getName() = 0;
};

#endif /* _FUNCTION_H_ */