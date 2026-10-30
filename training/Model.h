#ifndef _MODEL_H_
#define _MODEL_H_
#include <filesystem>
#include <random>
#include <vector>

/**
 * Rede neural em camadas densas: cada neurônio recebe todas as saídas da
 * camada anterior (a primeira recebe as entradas da rede), cada uma vezes um
 * peso, soma o bias e aplica a função de ativação da camada. As saídas da
 * rede são os neurônios da última camada. Lê e grava o arquivo da rede
 * (.bin); o formato antigo, em grafo, é lido quando os nós estão em camadas.
 */
class Model
{
public:
	/**
	 * Função de ativação da camada, mesmos valores gravados no arquivo
	 */
	enum Function
	{
		Linear = 0,
		Degrau,
		Bipolar,
		Logistic,
		Hiperbolic,
		Relu,
		// normaliza os neurônios da camada juntos, as saídas deles somam 1
		Softmax,
		FunctionCount
	};

	struct Layer
	{
		int size;
		int function;
		// inclinação da logística e da hiperbólica
		float beta;
		// peso da entrada i no neurônio j em weights[i * size + j], a linha de
		// cada entrada contígua para o cálculo pular as entradas em 0
		std::vector<float> weights;
		std::vector<float> bias;
	};

	// diferença máxima entre o valor e o alvo para considerar a saída certa
	static constexpr float Tolerance = 0.05f;
public:
	int inputs;
	// lado do quadrado de pixels que as entradas formam, 0 se não são imagem
	int side;
	std::vector<Layer> layers;
public:
	Model();

	/**
	 * Quantidade de entradas da camada: as da rede na primeira e o tamanho da
	 * anterior nas demais
	 */
	int inputCount(int layer) const;

	/**
	 * Neurônios da última camada, 0 sem camadas
	 */
	int outputs() const;

	/**
	 * Camada nova com os pesos e os bias em 0, inputCount(index) entradas
	 */
	static Layer layer(int inputs, int size, int function);

	/**
	 * Valor do neurônio com a função para a soma x, já com o bias. A softmax,
	 * que depende dos outros neurônios da camada, é calculada em activate.
	 */
	static float activate(int function, float beta, float x);

	/**
	 * Aplica a função da camada aos count x size valores, a softmax em cada
	 * linha
	 */
	static void activate(const Layer& layer, float* values, int count);

	/**
	 * Derivada da função de ativação em função da saída y do neurônio. Degrau e
	 * bipolar não têm derivada útil e usam 1, como na regra do perceptron. A
	 * softmax também usa 1: o erro dela fica sendo a diferença para o alvo, o
	 * da entropia cruzada, que não some quando a saída está longe do alvo.
	 */
	static float derivative(int function, float beta, float y);

	/**
	 * Calcula count amostras de uma vez, as entradas em linhas de inputs
	 * valores. values[l] recebe os count x size valores da camada l.
	 */
	void forward(const float* input, int count, std::vector<std::vector<float> >& values) const;

	/**
	 * Saídas da rede para uma amostra
	 */
	std::vector<float> run(const std::vector<float>& input) const;

	/**
	 * Sorteia os pesos e os bias de cada camada entre -r e r, com 2 casas, r
	 * = raiz de 6 / (entradas + neurônios) e no máximo 1
	 */
	void randomize(std::mt19937& random);
	static float randomWeight(std::mt19937& random, int inputs, int size);

	/**
	 * Indica se todos os pesos e bias são números finitos
	 */
	bool isFinite() const;

	/**
	 * Muda a quantidade de entradas da camada, mantendo os pesos das que
	 * ficam; as novas ganham pesos sorteados
	 */
	void resizeInputs(int index, int count, std::mt19937& random);

	/**
	 * Muda a quantidade de neurônios da camada e as entradas da seguinte; os
	 * neurônios novos ganham pesos e bias sorteados
	 */
	void resize(int index, int size, std::mt19937& random);

	/**
	 * Muda as entradas da rede e as da primeira camada
	 */
	void setInputs(int count, int side, std::mt19937& random);

	/**
	 * Camada nova na posição, ligada às vizinhas com pesos sorteados
	 */
	void insertLayer(int index, int size, int function, std::mt19937& random);

	/**
	 * Remove a camada, a seguinte passa a receber as saídas da anterior
	 */
	void removeLayer(int index, std::mt19937& random);

	/**
	 * Lado do quadrado de pixels que count entradas formam, 8 com 64, ou 0 se
	 * count não é um quadrado de pelo menos 2 x 2
	 */
	static int imageSide(int count);

	/**
	 * Lê a rede do arquivo no formato 2.0 ou, nos formatos 1.1 e 1.2 da rede
	 * em grafo, quando os nós estão em camadas: cada ligação vai de uma camada
	 * à seguinte, os neurônios de uma camada têm a mesma função e as saídas
	 * vêm da última. A ligação ausente vira peso 0 e a entrada de bias é
	 * somada ao bias dos neurônios que ela alimenta. Lança exceção se o
	 * arquivo for inválido ou a rede não estiver em camadas.
	 */
	static Model load(const std::filesystem::path& path);

	/**
	 * Grava a rede no formato 2.0. Lança exceção se falhar.
	 */
	void save(const std::filesystem::path& path) const;
};

#endif /* _MODEL_H_ */
