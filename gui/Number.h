#ifndef _NUMBER_H_
#define _NUMBER_H_
#include <string>

/**
 * Números como a interface os mostra e lê, com vírgula decimal. Sem o raylib,
 * para a linha de comando usar também.
 */
namespace number
{
	/**
	 * Número com vírgula decimal, no máximo digits casas e sem zeros à direita;
	 * a partir de um milhão em notação científica, com digits casas na mantissa
	 */
	std::string format(double value, int digits);

	/**
	 * Lê um número com vírgula ou ponto decimal, retorna false se o texto não
	 * começa com um número finito
	 */
	bool parse(const std::string& text, double& value);
}

#endif /* _NUMBER_H_ */
