#ifndef _PLATFORM_H_
#define _PLATFORM_H_
#include <filesystem>
#include <string>

/**
 * Caminhos são guardados em UTF-8, como o raylib os entrega. Estas funções
 * convertem para std::filesystem::path, já que no Windows um char* seria
 * interpretado na página de código ANSI.
 */
std::filesystem::path pathFromUtf8(const std::string& str);
std::string pathToUtf8(const std::filesystem::path& path);

/**
 * Caminho do arquivo da rede salva, dentro da pasta de dados do usuário
 * (no macOS ~/Library/Application Support/RedesNeurais). Cria a pasta se
 * necessário.
 */
std::string getDataFilePath();

/**
 * Caminho do arquivo com o estado da janela, na mesma pasta da rede salva
 */
std::string getSettingsFilePath();

#endif /* _PLATFORM_H_ */
