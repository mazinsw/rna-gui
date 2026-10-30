#ifndef _TRAINING_H_
#define _TRAINING_H_
#include "Network.h"
#include <filesystem>
#include <vector>

/**
 * Treina os pesos e os bias de uma Network por retropropagação. Cada treino
 * pode guardar a amostra atual da rede (os valores das entradas e os alvos das
 * saídas) e ajusta a rede para todas as amostras já guardadas, então treinar
 * uma combinação não desfaz as outras. Lê e grava as amostras ao lado do
 * arquivo da rede (rede.amostras.txt).
 */
class Training
{
public:
	/**
	 * Como terminou um treinamento
	 */
	enum Status
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
		// sem amostras: com softmax e nenhum alvo marcado a atual não entra
		NoTarget
	};

	struct Result
	{
		Status status;
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
	std::vector<Sample> samples;
private:
	/**
	 * Pesos das conexões seguidos dos bias dos neurônios, na ordem das listas
	 */
	std::vector<double> save(Network& network);
	void restore(Network& network, const std::vector<double>& values);

	/**
	 * Marca os pesos e os bias diferentes dos guardados, retorna true se algum
	 * mudou
	 */
	bool markChanged(Network& network, const std::vector<double>& values);

	/**
	 * Valores atuais das entradas e alvos das saídas
	 */
	Sample current(Network& network);

	/**
	 * Indica se a amostra tem as mesmas entradas e saídas das guardadas
	 */
	bool matches(const Sample& sample);

	/**
	 * Guarda a amostra, no lugar da que tem as mesmas entradas, retorna false
	 * se ela já estava guardada
	 */
	bool remember(const Sample& sample);

	/**
	 * Põe os valores da amostra nas entradas, menos nas de bias, e propaga
	 */
	void apply(Network& network, const Sample& sample);

	/**
	 * Propaga cada amostra e soma em gradient, na ordem de save(), o sentido
	 * em que cada peso e bias reduz o erro
	 */
	Evaluation evaluate(Network& network, std::vector<double>& gradient);
public:
	// ajusta os bias dos neurônios, além dos pesos
	bool biases;
public:
	Training();

	/**
	 * Guarda a amostra atual e treina até as saídas de todas as amostras
	 * ficarem iguais aos alvos ou chegar ao limite de iterações, marca os pesos
	 * e os bias alterados. Cada peso e bias tem um passo próprio (Rprop), que
	 * cresce enquanto o sentido do ajuste se mantém e cai à metade quando ele
	 * troca. A rede termina com os melhores pesos encontrados, nunca pior do
	 * que começou, e com os valores da amostra atual. Com softmax, cujas saídas
	 * somam 1, a amostra atual sem nenhum alvo diferente de 0 não é guardada.
	 * Com withScreen false treina só as amostras guardadas, sem a atual.
	 */
	Result train(Network& network, bool withScreen = true);

	/**
	 * Esquece as amostras guardadas, o próximo treino usa só a atual. Deve ser
	 * chamado quando as entradas ou as saídas mudam.
	 */
	void forgetSamples();
	int getSampleCount();
	const std::vector<Sample>& getSamples();

	/**
	 * Substitui as amostras guardadas, que devem ter os valores de todas as
	 * entradas e os alvos de todas as saídas
	 */
	void setSamples(const std::vector<Sample>& samples);

	/**
	 * Arquivo das amostras ao lado da rede: rede.bin -> rede.amostras.txt
	 */
	static std::filesystem::path samplesPath(const std::filesystem::path& network);

	/**
	 * Grava as amostras ao lado do arquivo da rede; sem amostras apaga o
	 * arquivo delas. Lança exceção se falhar.
	 */
	void saveSamples(const std::filesystem::path& network);

	/**
	 * Lê as amostras gravadas ao lado do arquivo da rede, se ele existir, e
	 * retorna quantas leu. Se elas não combinam com as entradas e as saídas da
	 * rede lança exceção e ficam sem amostras.
	 */
	int loadSamples(const std::filesystem::path& file, const Network& network);
};

#endif /* _TRAINING_H_ */
