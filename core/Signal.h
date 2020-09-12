#ifndef _SIGNAL_H_
#define _SIGNAL_H_
#include <list>
#include <map>

class Signal;

/**
 * Classe ponte para conectar um sinal a outro com informa��o de peso
 */
class Connection
{
private:
	Signal * source;
	Signal * destination;
	double weight;
public:
	Connection(Signal * source, Signal * destination, double weight)
	: source(source), destination(destination), weight(weight)
	{}

	/**
	 * Obt�m o sinal de origem
	 */
	inline Signal * getSource()
	{
		return source;
	}
	
	/**
	 * Obt�m o sinal de destino
	 */
	inline Signal * getDestination()
	{
		return destination;
	}
	
	/**
	 * Obt�m o peso da conex�o
	 */
	inline double getWeight()
	{
		return weight;
	}
	
	/**
	 * Ajusta a peso da conex�o
	 */
	inline void setWeight(double weight)
	{
		this->weight = weight;
	}	
};

/**
 * Classe que emite um sinal na rede
 */
class Signal
{
private:
	/**
	 * lista de conex�es que saem deste sinal
	 */
	std::list<Connection*> connections;
protected:
	/**
	 * Recebe o sinal de uma conex�o
	 */
	virtual void receive(Connection* connection, double value);
	
	/**
	 * Devolve o valor calculado do sinal
	 */
	virtual double getValue() = 0;
	
	/**
	 * Evento de que algum sinal se conectou a esse
	 */
	virtual void onConnect(Signal * signal){}
	
	/**
	 * Evento em que um sinal foi desconectado
	 */
	virtual void onDiconnect(Signal * signal){}
public:
	virtual ~Signal();
	
	/**
	 * Propaga o sinal para a pr�xima conex�o
	 */
	void propagate();
	
	/**
	 * Conecta esse sinal a outro 
	 */
	void connect(Signal * signal, double weight);
	
	/**
	 * Desconecta esse sinal de outro informado
	 */
	void disconnect(Signal * signal);
	
	/**
	 * Procura uma conex�o ao sinal informado
	 */
	Connection* findConnection(Signal * signal);
	
	/**
	 * Obt�m a lista com todas as conex�es de saida desse sinal
	 */
	inline const std::list<Connection*>& getConnections()
	{
		return connections;
	}
	
	/**
	 * Ajusta o peso de uma conex�o
	 */
	void setWeight(Signal* signal, double weight);
	
	/**
	 * Obt�m o peso de uma conex�o
	 */
	double getWeight(Signal* signal);
};

#endif /* _SIGNAL_H_ */