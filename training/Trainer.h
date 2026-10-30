#ifndef _TRAINER_H_
#define _TRAINER_H_
#include "Dataset.h"
#include "Model.h"
#include <atomic>
#include <functional>
#include <vector>

/**
 * Treina os pesos e os bias de um Model por retropropagação. Com poucas
 * amostras usa todas a cada passo (Rprop) até as saídas chegarem aos alvos;
 * com muitas, que demorariam a cada passo, usa lotes sorteados (SGD) por um
 * número de épocas. A última camada com softmax usa o erro da entropia
 * cruzada e as demais o erro quadrático.
 */
class Trainer
{
public:
	/**
	 * Como terminou um treinamento
	 */
	enum Status
	{
		// Rprop: todas as saídas iguais aos alvos
		Trained,
		OnTarget,
		NotConverged,
		// nenhum passo, por menor que fosse, reduziu o erro
		Stalled,
		// SGD: fez todas as épocas
		Finished,
		// parado por stop, com os pesos até ali
		Stopped,
		// a rede já tinha pesos ou bias que não são números finitos, ou o SGD
		// divergiu e voltou aos da época anterior
		Invalid,
		NoOutput,
		NoSamples,
		// as amostras não têm as entradas e as saídas da rede
		Mismatch
	};

	/**
	 * Andamento, avisado a cada iteração do Rprop e a cada época do SGD
	 */
	struct Progress
	{
		bool epochs;
		// iteração ou época, de 1 em diante, e o máximo
		int step, steps;
		// Rprop: soma das diferenças entre o alvo e o valor das saídas em
		// todas as amostras; SGD: média por amostra na época
		double error;
		// fração das amostras com rótulo cuja maior saída é a do rótulo, no
		// treino e no teste, -1 sem rótulos
		double accuracy, testAccuracy;
	};

	struct Result
	{
		Status status;
		// treinou por épocas (SGD)
		bool epochs;
		// iterações ou épocas
		int steps;
		double errorBefore, errorAfter;
		double accuracy, testAccuracy;
		int samples;
		// algum peso ou bias mudou
		bool changed;
	};

	// a partir daí o treino é por épocas
	static constexpr int MaxFullBatch = 2000;
	// Rprop: passo inicial e máximo de cada peso e bias, que se adapta a cada
	// iteração; abaixo do mínimo o passo já não muda a rede e o treino para
	static constexpr float InitialStep = 0.05f;
	static constexpr float MaxStep = 1;
	static constexpr float MinStep = 1e-9f;
	static constexpr int MaxIterations = 1000;
private:
	/**
	 * Erro de um conjunto de amostras
	 */
	struct Evaluation
	{
		// soma dos quadrados das diferenças, o que o treino reduz
		double squared;
		// soma das diferenças, a mostrada
		double absolute;
		// todas as saídas de todas as amostras estão iguais aos alvos
		bool onTarget;
		int right, labeled;
	};

	/**
	 * Um valor por peso e bias, na forma das camadas
	 */
	struct Parameters
	{
		std::vector<std::vector<float> > weights, bias;
	};
private:
	// valores e erros de cada camada no último cálculo
	std::vector<std::vector<float> > values, deltas;
private:
	/**
	 * Parâmetros com o tamanho das camadas, todos em value
	 */
	static Parameters parameters(const Model& model, float value);
	static void clear(Parameters& parameters);

	/**
	 * Indica se algum peso ou bias das redes, de camadas iguais, é diferente
	 */
	static bool differs(const Model& model, const Model& other);

	/**
	 * Propaga count amostras, entradas em input e alvos em target, e soma em
	 * gradient o sentido em que cada peso e bias reduz o erro, vezes scale
	 */
	Evaluation evaluate(const Model& model, const float* input, const float* target, int count,
		Parameters& gradient, float scale);

	/**
	 * Indica se o bias do neurônio fica como está
	 */
	bool isFixed(int layer, int neuron);
	Result rprop(Model& model, const Dataset& samples);
	Result sgd(Model& model, const Dataset& samples, const Dataset* test);
public:
	// SGD: épocas, taxa de aprendizado e amostras por lote
	int epochs;
	float rate;
	int batch;
	// semente do sorteio dos lotes, o mesmo treino dá a mesma rede
	unsigned seed;
	// bias que o treino não muda, por camada e neurônio; vazio muda todos
	std::vector<std::vector<char> > fixedBiases;
	// chamado na thread do treino
	std::function<void(const Progress&)> progress;
	// pede para parar, de outra thread; o treino não o volta para false
	std::atomic<bool> stop;
public:
	Trainer();

	/**
	 * Indica se count amostras são treinadas por épocas
	 */
	static bool byEpochs(int count);

	/**
	 * Treina a rede com as amostras; o SGD mostra o acerto no teste a cada
	 * época, se houver. O Rprop treina até as saídas de todas as amostras
	 * ficarem iguais aos alvos ou chegar ao limite de iterações, cada peso e
	 * bias com um passo próprio, que cresce enquanto o sentido do ajuste se
	 * mantém e cai à metade quando ele troca; a rede termina com os melhores
	 * pesos encontrados, nunca pior do que começou.
	 */
	Result train(Model& model, const Dataset& samples, const Dataset* test = nullptr);

	/**
	 * Sentido em que cada peso e bias reduz o erro, somado em todas as
	 * amostras, na forma da rede: o que o treino usa, para conferir as
	 * derivadas
	 */
	Model gradient(const Model& model, const Dataset& samples);

	/**
	 * Fração das amostras com rótulo cuja maior saída é a do rótulo, -1 sem
	 * rótulos ou com uma saída só
	 */
	static double accuracy(const Model& model, const Dataset& samples);

	/**
	 * Saída de maior valor, ou -1 sem saídas ou com valores inválidos
	 */
	static int recognized(const float* outputs, int count);
};

#endif /* _TRAINER_H_ */
