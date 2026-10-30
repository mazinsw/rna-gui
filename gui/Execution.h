#ifndef _EXECUTION_H_
#define _EXECUTION_H_
#include <map>
#include <vector>
#include "Node.h"
#include "Edge.h"

class NeuronNode;
class OutputNode;

/**
 * Executa a rede desenhada: propaga os valores das entradas até as saídas e
 * treina os pesos por retropropagação. Cada treino guarda a amostra atual (os
 * valores das entradas e os alvos das saídas) e ajusta a rede para todas as
 * amostras já treinadas, então treinar uma combinação não desfaz as outras.
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
		NoOutput,
		// com softmax e nenhum alvo marcado na tela não há o que treinar
		NoTarget
	};

	struct TrainResult
	{
		TrainStatus status;
		int iterations;
		// soma das diferenças entre o alvo e o valor das saídas em todas as
		// amostras
		double errorBefore;
		double errorAfter;
		// algum peso ou bias mudou
		bool changed;
		// amostras usadas, contando a atual
		int samples;
		// a amostra atual entrou na lista ou mudou os alvos de uma guardada
		bool samplesChanged;
	};

	/**
	 * Combinação treinada: o valor de cada entrada e o alvo de cada saída, na
	 * ordem dos nós. A entrada de bias fica sempre em 1.
	 */
	struct Sample
	{
		std::vector<double> inputs;
		std::vector<double> targets;
	};

	// passo inicial e máximo de cada peso e bias, que se adapta a cada iteração
	static constexpr double InitialStep = 0.05;
	static constexpr double MaxStep = 1;
	// abaixo dele o passo já não muda a rede e o treino para
	static constexpr double MinStep = 1e-9;
	static constexpr int MaxIterations = 1000;
private:
	/**
	 * Erro da rede em todas as amostras
	 */
	struct Evaluation
	{
		// soma dos quadrados das diferenças, o que o treino reduz
		double squared;
		// soma das diferenças, a mostrada
		double absolute;
		// todas as saídas de todas as amostras estão iguais aos alvos
		bool onTarget;
	};
private:
	std::vector<Node*>& nodes;
	std::vector<Edge*>& edges;
	// índices em nodes dos nós em ordem topológica, cada um depois dos que o
	// alimentam
	std::vector<int> order;
	// índices das conexões que chegam e que saem de cada nó
	std::vector<std::vector<int> > incoming;
	std::vector<std::vector<int> > outgoing;
	// índices da origem e do destino de cada conexão
	std::vector<int> sources;
	std::vector<int> targets;
	// nós de cada tipo na ordem da lista
	std::vector<Node*> inputs;
	std::vector<OutputNode*> outputs;
	std::vector<NeuronNode*> neurons;
	// índices das saídas e dos neurônios acima
	std::vector<int> outputIndexes;
	std::vector<int> neuronIndexes;
	// índices dos neurônios softmax, calculados juntos
	std::vector<int> softmax;
	std::vector<Sample> samples;
	bool cycle;
private:
	void sort();

	/**
	 * Põe o nó na ordem depois dos que o alimentam. Os neurônios softmax entram
	 * juntos, depois de tudo o que alimenta qualquer um deles.
	 */
	void visit(int node, std::vector<int>& state);
	bool isSoftmax(int node);

	/**
	 * Soma dos valores que chegam ao nó, cada um vezes o peso da conexão
	 */
	double sum(int node);

	/**
	 * Calcula o valor de cada neurônio e saída na ordem já montada
	 */
	void propagate();
	bool hasOutput();

	/**
	 * Indica se a amostra tem as mesmas entradas e saídas das guardadas
	 */
	bool matches(const Sample& sample);

	/**
	 * Pesos das conexões seguidos dos bias dos neurônios, na ordem das listas
	 */
	std::vector<double> save();
	void restore(const std::vector<double>& values);

	/**
	 * Marca os pesos e os bias diferentes dos guardados, retorna true se algum
	 * mudou
	 */
	bool markChanged(const std::vector<double>& values);

	/**
	 * Valores atuais das entradas e alvos das saídas
	 */
	Sample current();

	/**
	 * Guarda a amostra, no lugar da que tem as mesmas entradas, retorna false
	 * se ela já estava guardada
	 */
	bool remember(const Sample& sample);

	/**
	 * Põe os valores da amostra nas entradas, menos nas de bias, e propaga
	 */
	void apply(const Sample& sample);

	/**
	 * Propaga cada amostra e soma em gradient, na ordem de save(), o sentido
	 * em que cada peso e bias reduz o erro
	 */
	Evaluation evaluate(std::vector<double>& gradient);
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
	 * Guarda a amostra atual e treina até as saídas de todas as amostras
	 * ficarem iguais aos alvos ou chegar ao limite de iterações, marca os pesos
	 * e os bias alterados. Cada peso e bias tem um passo próprio (Rprop), que
	 * cresce enquanto o sentido do ajuste se mantém e cai à metade quando ele
	 * troca. A rede termina com os melhores pesos encontrados, nunca pior do
	 * que começou. Com softmax, cujas saídas somam 1, a amostra atual sem
	 * nenhum alvo diferente de 0 não é guardada.
	 */
	TrainResult train();

	/**
	 * Esquece as amostras guardadas, o próximo treino usa só a atual. Deve ser
	 * chamado quando as entradas ou as saídas mudam.
	 */
	inline void forgetSamples()
	{
		samples.clear();
	}

	inline int getSampleCount()
	{
		return (int)samples.size();
	}

	inline const std::vector<Sample>& getSamples()
	{
		return samples;
	}

	/**
	 * Substitui as amostras guardadas, que devem ter os valores de todas as
	 * entradas e os alvos de todas as saídas
	 */
	inline void setSamples(const std::vector<Sample>& samples)
	{
		this->samples = samples;
	}
};

#endif /* _EXECUTION_H_ */
