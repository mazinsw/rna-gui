#ifndef _PIXELS_H_
#define _PIXELS_H_
#include "../training/Dataset.h"
#include <string>
#include <vector>

/**
 * Imagem reduzida para as entradas da rede: side x side valores de 0 (fundo)
 * a 1 (traço), linha a linha de cima para baixo. A mesma redução serve às
 * imagens arrastadas para a janela e às amostras dos exemplos, para a rede
 * receber na execução o que recebeu no treino.
 */
namespace pixels
{
	/**
	 * Reduz a imagem em tons de cinza (0 preto, 1 branco), linha a linha. Como
	 * no MNIST: o fundo claro é invertido para o traço ficar claro, o contraste
	 * vai de 0 a 1 e o traço é recortado, centralizado num quadrado e reduzido
	 * pela média dos pixels para caber dentro de uma borda vazia de 1/7 do
	 * lado, os 4 pixels do MNIST em 28 x 28 e 1 em 8 x 8. Os valores têm 3
	 * casas. Uma imagem de uma cor só dá tudo 0.
	 */
	std::vector<double> fromGray(const std::vector<float>& gray, int width, int height,
		int side);

	/**
	 * Lê a imagem do arquivo e a reduz, o caminho é UTF-8. A transparência
	 * fica branca. Lança exceção se o arquivo não for uma imagem que o raylib
	 * lê.
	 */
	std::vector<double> fromFile(const std::string& fileName, int side);

	/**
	 * Amostras das imagens da pasta com o dígito no começo do nome (3_7.png),
	 * em ordem de nome, reduzidas como em fromFile, com classes saídas e o
	 * alvo do dígito em 1. Lança exceção se a pasta não tiver imagens assim ou
	 * se um dígito passar das saídas.
	 */
	Dataset fromFolder(const std::string& folder, int side, int classes);

	/**
	 * Imagens e rótulos do MNIST (formato IDX, veja Dataset::loadIdx) com a
	 * redução de fromGray, para a rede treinada com elas receber o mesmo que
	 * das imagens arrastadas. Lança exceção se falhar.
	 */
	Dataset fromIdx(const std::filesystem::path& path, int side);

	/**
	 * Indica se a extensão do arquivo é de imagem
	 */
	bool isImage(const std::string& fileName);

	/**
	 * Dígito que começa o nome do arquivo, 3 em "3_7.png", ou -1 se o nome
	 * não começa com um só dígito
	 */
	int labelOf(const std::string& fileName);
}

#endif /* _PIXELS_H_ */
