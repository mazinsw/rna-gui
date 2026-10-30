#ifndef _EXECUTION_H_
#define _EXECUTION_H_
#include <filesystem>
#include <vector>
#include "Node.h"
#include "Edge.h"
#include "../training/Graph.h"
#include "../training/Training.h"

/**
 * Liga a rede desenhada à do módulo de treino: antes de executar ou treinar
 * copia os nós e as conexões para uma Graph e depois traz de volta os
 * valores e, do treino, os pesos, os bias e as marcas de alterado. Guarda as
 * amostras do treino.
 */
class Execution
{
private:
	std::vector<Node*>& nodes;
	std::vector<Edge*>& edges;
	Graph network;
	Training training;
private:
	/**
	 * Copia a rede desenhada para network
	 */
	void copyIn();

	/**
	 * Traz de volta os valores dos nós e, com trained, os pesos, os bias e as
	 * marcas de alterado
	 */
	void copyOut(bool trained);
public:
	Execution(std::vector<Node*>& nodes, std::vector<Edge*>& edges);

	/**
	 * Calcula o valor de cada neurônio e saída, retorna false se a rede tem
	 * ciclo
	 */
	bool execute();

	inline bool hasCycle()
	{
		return network.hasCycle();
	}

	/**
	 * Remove a marca de alterado dos pesos e dos bias
	 */
	void clearChanged();

	/**
	 * Indica, na ordem das conexões, se o treino ajusta o peso, veja
	 * Graph::isTrainable
	 */
	std::vector<bool> isTrainable();

	/**
	 * Soma das diferenças entre o alvo e o valor de cada saída
	 */
	double getError();

	/**
	 * Indica se todas as saídas estão iguais aos alvos
	 */
	bool isOnTarget();

	/**
	 * Põe a imagem nas entradas e o rótulo nos alvos e executa, veja
	 * Graph::showImage
	 */
	bool showImage(const std::vector<double>& values, int label);

	/**
	 * Treina com a combinação da tela e as amostras guardadas, ou só com elas,
	 * veja Training::train
	 */
	Training::Result train(bool withScreen = true);

	/**
	 * Rede desenhada, para gravar
	 */
	Graph& getNetwork();

	inline void forgetSamples()
	{
		training.forgetSamples();
	}

	inline int getSampleCount()
	{
		return training.getSampleCount();
	}

	/**
	 * Lê as amostras gravadas ao lado do arquivo da rede, veja
	 * Training::loadSamples
	 */
	int loadSamples(const std::filesystem::path& file);
	void saveSamples(const std::filesystem::path& file);
};

#endif /* _EXECUTION_H_ */
