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
		// nenhum passo, por menor que fosse, reduziu o erro
		Stalled,
		// a rede já tinha pesos, bias ou valores que não são números finitos
		Invalid,
		HasCycle,
		NoOutput
	};

	struct TrainResult
	{
		TrainStatus status;
		int iterations;
		double errorBefore;
		double errorAfter;
		// algum peso ou bias mudou
		bool changed;
	};

	// taxa de aprendizado inicial, ajustada a cada iteração
	static constexpr double LearningRate = 0.1;
	// abaixo dela o passo já não muda a rede e o treino para
	static constexpr double MinLearningRate = 1e-12;
	static constexpr int MaxIterations = 1000;
private:
	/**
	 * Pesos das conexões e bias dos neurônios, na ordem das listas
	 */
	struct Snapshot
	{
		std::vector<double> weights;
		std::vector<double> biases;
	};
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
	 * Soma dos quadrados das diferenças entre o alvo e o valor de cada saída,
	 * o erro que a retropropagação reduz
	 */
	double squaredError();
	Snapshot save();
	void restore(const Snapshot& snapshot);

	/**
	 * Marca os pesos e os bias diferentes dos guardados, retorna true se algum
	 * mudou
	 */
	bool markChanged(const Snapshot& snapshot);

	/**
	 * Ajusta os pesos e os bias uma vez com a taxa informada, retorna false sem
	 * alterar a rede se algum deixaria de ser um número finito
	 */
	bool step(double rate);
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
	 * Remove a marca de alterado dos pesos e dos bias
	 */
	void clearChanged();

	/**
	 * Indica, na ordem das conexões, se o treino ajusta o peso: as que chegam
	 * nos neurônios e as que chegam numa saída que soma mais de um nó. A saída
	 * ligada a um nó só repassa o valor dele e esse peso fica fixo.
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
	 * Treina até as saídas ficarem iguais aos alvos ou chegar ao limite de
	 * iterações, marca os pesos e os bias alterados. A taxa cresce enquanto o
	 * erro cai; o passo que aumenta o erro é desfeito e a taxa cai à metade,
	 * então a rede nunca termina pior do que começou.
	 */
	TrainResult train();
};

#endif /* _EXECUTION_H_ */
