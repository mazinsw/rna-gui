#ifndef _DATASET_H_
#define _DATASET_H_
#include <filesystem>
#include <vector>

/**
 * Amostras de treino: os valores das entradas e os alvos das saídas de cada
 * uma. Lê e grava as amostras ao lado do arquivo da rede (rede.amostras.txt)
 * e lê as imagens e os rótulos do MNIST (formato IDX).
 */
class Dataset
{
private:
	// count x inputs valores, ou em bytes de 0 a 255 para 0 a 1 nas imagens,
	// que ocupam um quarto da memória
	std::vector<float> values;
	std::vector<unsigned char> bytes;
	// count x outputs
	std::vector<float> targets;
public:
	int inputs, outputs;
public:
	Dataset(int inputs = 0, int outputs = 0);

	int size() const;
	bool empty() const;
	void clear();

	/**
	 * Acrescenta a amostra, com inputs valores e outputs alvos
	 */
	void add(const std::vector<float>& input, const std::vector<float>& target);

	/**
	 * Acrescenta a amostra de uma imagem, com os valores de 0 a 1 guardados
	 * em bytes, que ocupam um quarto da memória
	 */
	void addImage(const std::vector<float>& pixels, const std::vector<float>& target);

	/**
	 * Guarda a amostra no lugar da que tem as mesmas entradas, ou no fim,
	 * retorna false se ela já estava guardada
	 */
	bool remember(const std::vector<float>& input, const std::vector<float>& target);

	/**
	 * Copia os valores das entradas da amostra para out
	 */
	void input(int sample, float* out) const;
	std::vector<float> input(int sample) const;
	const float* target(int sample) const;

	/**
	 * Saída com o único alvo diferente de 0, ou -1
	 */
	int label(int sample) const;

	/**
	 * Arquivo das amostras ao lado da rede: rede.bin -> rede.amostras.txt
	 */
	static std::filesystem::path samplesPath(const std::filesystem::path& network);

	/**
	 * Grava as amostras ao lado do arquivo da rede; sem amostras apaga o
	 * arquivo delas. Lança exceção se falhar.
	 */
	void saveSamples(const std::filesystem::path& network) const;

	/**
	 * Lê as amostras gravadas ao lado do arquivo da rede, vazio se ele não
	 * existir. Lança exceção se elas não tiverem inputs entradas e outputs
	 * saídas.
	 */
	static Dataset loadSamples(const std::filesystem::path& network, int inputs, int outputs);

	/**
	 * Indica se o caminho é um arquivo de imagens IDX ou uma pasta com o
	 * train-images-idx3-ubyte
	 */
	static bool isIdx(const std::filesystem::path& path);

	/**
	 * Lê as imagens IDX e os rótulos ao lado (images-idx3 -> labels-idx1),
	 * cada pixel uma entrada e cada rótulo uma saída com alvo 1. De uma pasta
	 * lê o train-images-idx3-ubyte. Lança exceção se falhar.
	 */
	static Dataset loadIdx(const std::filesystem::path& path);

	/**
	 * Conjunto de teste ao lado do de treino, train-* -> t10k-*, ou vazio se
	 * não houver
	 */
	static std::filesystem::path testPath(const std::filesystem::path& path);
};

#endif /* _DATASET_H_ */
