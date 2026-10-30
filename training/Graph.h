#ifndef _GRAPH_H_
#define _GRAPH_H_
#include <filesystem>
#include <utility>
#include <vector>

class Training;

/**
 * Rede neural sem interface: os nós, as conexões e a execução, que calcula o
 * valor de cada neurônio e saída a partir das entradas. Lê e grava o arquivo
 * da rede (.bin). A interface copia para ela a rede desenhada; a linha de
 * comando e o console a usam direto.
 */
class Graph
{
public:
	/**
	 * Tipo do nó, mesmos valores gravados no arquivo
	 */
	enum Type
	{
		Neuron = 1,
		Input,
		Output
	};

	/**
	 * Função de ativação do neurônio, mesmos valores gravados no arquivo
	 */
	enum Function
	{
		Linear = 0,
		Degrau,
		Bipolar,
		Logistic,
		Hiperbolic,
		Relu,
		// normaliza os neurônios softmax da rede juntos, as saídas deles somam 1
		Softmax,
		FunctionCount
	};

	struct Node
	{
		int type;
		int function;
		// inclinação da logística e da hiperbólica
		double beta;
		// somado à entrada da função de ativação do neurônio
		double bias;
		// entrada de bias, que vale sempre 1
		bool biasInput;
		// posição no desenho, gravada no arquivo
		int x, y;
		// valor da entrada, ou calculado no neurônio e na saída
		double value;
		// valor desejado na saída
		double target;
		// bias alterado pelo último treino
		bool biasChanged;
	};

	struct Edge
	{
		// índices dos nós
		int source, target;
		double weight;
		// peso alterado pelo último treino
		bool changed;
	};

	// diferença máxima entre o valor e o alvo para considerar a saída certa
	static constexpr double Tolerance = 0.05;
public:
	std::vector<Node> nodes;
	std::vector<Edge> edges;
private:
	friend class Training;
	// índices dos nós em ordem topológica, cada um depois dos que o alimentam
	std::vector<int> order;
	// conexões que chegam ao nó n: incomingEdges[incomingStart[n]] até antes
	// de incomingEdges[incomingStart[n + 1]], na ordem da lista; o mesmo para
	// as que saem
	std::vector<int> incomingStart, incomingEdges;
	std::vector<int> outgoingStart, outgoingEdges;
	// índices dos nós de cada tipo na ordem da lista
	std::vector<int> inputs, outputs, neurons;
	// índices dos neurônios softmax, calculados juntos
	std::vector<int> softmax;
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
public:
	Graph();

	/**
	 * Nó novo do tipo, com valores iniciais: neurônio linear sem bias, beta 1,
	 * na posição 0, 0
	 */
	static Node node(int type, int function = Linear);

	/**
	 * Indica se uma conexão do tipo de nó source ao tipo target é permitida:
	 * entradas e saídas não ligam entre si
	 */
	static bool canConnect(int source, int target);

	/**
	 * Valor do neurônio com a função para a soma x, já com o bias. A softmax,
	 * que depende dos outros neurônios, é calculada em execute.
	 */
	static double activate(int function, double beta, double x);

	/**
	 * Derivada da função de ativação em função da saída y do neurônio. Degrau e
	 * bipolar não têm derivada útil e usam 1, como na regra do perceptron. A
	 * softmax também usa 1: o erro dela fica sendo a diferença para o alvo, o
	 * da entropia cruzada, que não some quando a saída está longe do alvo.
	 */
	static double derivative(int function, double beta, double y);

	/**
	 * Monta a ordem de execução e calcula o valor de cada neurônio e saída,
	 * retorna false se a rede tem ciclo
	 */
	bool execute();

	/**
	 * Indica se a rede tinha ciclo na última execução
	 */
	bool hasCycle();

	/**
	 * Indica, na ordem das conexões, se o treino ajusta o peso: as que chegam
	 * nos neurônios e as que chegam numa saída que soma mais de um nó. A saída
	 * ligada a um nó só repassa o valor dele e esse peso fica fixo.
	 */
	std::vector<bool> isTrainable();

	/**
	 * Índices dos pontos na ordem de leitura: de cima para baixo e, na mesma
	 * altura, da esquerda para a direita
	 */
	static std::vector<int> readingOrder(const std::vector<std::pair<int, int> >& positions);

	/**
	 * Lado do quadrado de pixels que count entradas formam, 8 com 64, ou 0 se
	 * count não é um quadrado de pelo menos 2 x 2
	 */
	static int imageSide(int count);

	/**
	 * Índices dos nós do tipo na ordem de leitura das posições, sem as
	 * entradas de bias
	 */
	std::vector<int> readingOrder(int type);

	/**
	 * Lado do quadrado de pixels que as entradas de dado formam, ou 0
	 */
	int getImageSide();

	/**
	 * Põe os pixels da imagem nas entradas de dado, na ordem de leitura delas,
	 * e executa a rede. O alvo da saída do rótulo (a primeira de cima é a 0)
	 * fica em 1 e os das outras em 0; sem rótulo, ou com um além das saídas,
	 * todos ficam em 0. Retorna true se o rótulo marcou um alvo.
	 */
	bool showImage(const std::vector<double>& values, int label);

	/**
	 * Saída de maior valor, contando de 0 na ordem de leitura, ou -1 sem
	 * saídas ou com valores inválidos
	 */
	int getRecognized();

	/**
	 * Valor da saída, contando de 0 na ordem de leitura
	 */
	double getOutputValue(int output);

	/**
	 * Lê a rede do arquivo, nos formatos 1.1 e 1.2. As conexões que a rede não
	 * permite (repetidas, de um nó a ele mesmo ou entre entradas e saídas) são
	 * ignoradas. Lança exceção se o arquivo for inválido.
	 */
	static Graph load(const std::filesystem::path& path);

	/**
	 * Grava a rede no formato 1.1, que a versão Windows original abre, ou no
	 * 1.2 quando ela usa algum bias, entrada de bias, ReLU ou softmax. Lança
	 * exceção se falhar.
	 */
	void save(const std::filesystem::path& path);
};

#endif /* _GRAPH_H_ */
