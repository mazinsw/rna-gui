#ifndef _EXECUTION_H_
#define _EXECUTION_H_
#include <map>
#include <vector>
#include "Node.h"
#include "Edge.h"

/**
 * Executa a rede desenhada: propaga os valores das entradas até as saídas e
 * treina os pesos por retropropagação com a amostra atual (os valores das
 * entradas e os alvos das saídas)
 */
class Execution
{
public:
	/**
	 * Como terminou um treinamento
	 */
	enum TrainStatus
	{
		Trained,
		OnTarget,
		NotConverged,
		HasCycle,
		NoOutput
	};

	struct TrainResult
	{
		TrainStatus status;
		int iterations;
		double errorBefore;
		double errorAfter;
	};

	static constexpr double LearningRate = 0.1;
	static constexpr int MaxIterations = 1000;
private:
	std::vector<Node*>& nodes;
	std::vector<Edge*>& edges;
	// nós em ordem topológica, cada um depois dos que o alimentam
	std::vector<Node*> order;
	std::map<Node*, std::vector<Edge*> > incoming;
	std::map<Node*, std::vector<Edge*> > outgoing;
	bool cycle;
private:
	void sort();
	void visit(Node* node, std::map<Node*, int>& state);
	bool hasOutput();
	/**
	 * Ajusta os pesos uma vez, retorna false se algum peso deixaria de ser
	 * um número finito
	 */
	bool step();
public:
	Execution(std::vector<Node*>& nodes, std::vector<Edge*>& edges);

	/**
	 * Calcula o valor de cada neurônio e saída, retorna false se a rede tem
	 * ciclo
	 */
	bool execute();

	inline bool hasCycle()
	{
		return cycle;
	}

	/**
	 * Soma das diferenças entre o alvo e o valor de cada saída
	 */
	double getError();

	/**
	 * Indica se todas as saídas estão iguais aos alvos
	 */
	bool isOnTarget();

	/**
	 * Treina até as saídas ficarem iguais aos alvos ou chegar ao limite de
	 * iterações, marca os pesos alterados
	 */
	TrainResult train();
};

#endif /* _EXECUTION_H_ */
