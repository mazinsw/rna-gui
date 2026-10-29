#ifndef _PLATFORM_H_
#define _PLATFORM_H_
#include <string>

/**
 * Caminho do arquivo da rede salva, dentro da pasta de dados do usuário
 * (no macOS ~/Library/Application Support/RedesNeurais). Cria a pasta se
 * necessário.
 */
std::string getDataFilePath();

#endif /* _PLATFORM_H_ */
