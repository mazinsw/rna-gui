#ifndef _NEURON_H_
#define _NEURON_H_
#include "Signal.h"
#include "Function.h"
#include <vector>
#include <map>

class Neuron: public Signal
{
private:
	Function * function; // fun��o de ativa��o do neur�nio
	unsigned int connectedCount; // quantos sinais est�o conectados a esse
	double u; // total do processamento
	unsigned int processedCount; // quantidade de sinais processados
protected:
	/**
	 * Soma todos as entradas, aplica a fun��o e emite o sinal 
	 */
	virtual void receive(Connection* connection, double value);
	
	/**
	 * retorna o valor do processamento aplicado � fun��o de ativa��o
	 */
	virtual double getValue()
	{
		return function->execute(u);
	}
	
	/**
	 * adiciona na lista de sinais de entrada
	 */
	virtual void onConnect(Signal* signal);
	
	/**
	 * remove da lista de sinais de entrada
	 */
	virtual void onDiconnect(Signal* signal);
public:
	/**
	 * Inicializa vari�veis
	 */
	Neuron();
	
	/**
	 * Informa a fun��o de ativa��o do neur�nio
	 */
	inline void setFunction(Function * function)
	{
		this->function = function;
	}
	
	/**
	 * Retorna a fun��o de ativa��o
	 */
	inline Function* getFunction()
	{
		return function;
	}
};

#endif /* _NEURON_H_ */