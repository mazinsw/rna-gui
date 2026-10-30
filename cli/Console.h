#ifndef _CONSOLE_H_
#define _CONSOLE_H_
#include <string>
#include <vector>

namespace console
{
	/**
	 * Argumentos do programa em UTF-8. No Windows o argv vem na página de
	 * código ANSI, então eles são lidos de novo da linha de comando em UTF-16
	 * e o console passa a mostrar UTF-8; nos outros sistemas são o argv.
	 */
	std::vector<std::string> arguments(int argc, char *argv[]);
}

#endif /* _CONSOLE_H_ */
