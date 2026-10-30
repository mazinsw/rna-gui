#ifndef _CONSOLE_H_
#define _CONSOLE_H_
#include <string>
#include <vector>

namespace console
{
	/**
	 * Argumentos do programa em UTF-8. No Windows o argv vem na página de
	 * código ANSI, então eles são lidos de novo da linha de comando em UTF-16;
	 * nos outros sistemas são o argv.
	 */
	std::vector<std::string> arguments(int argc, char *argv[]);

	/**
	 * Faz o console do Windows mostrar o texto UTF-8 que o programa escreve,
	 * nos outros sistemas ele já mostra
	 */
	void showUtf8();
}

#endif /* _CONSOLE_H_ */
