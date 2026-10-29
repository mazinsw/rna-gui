#ifndef _RESOURCES_H_
#define _RESOURCES_H_
#include <raylib.h>

/**
 * Recursos embutidos no executável (imagens e fonte), substituem o
 * Resources.rc da versão Windows
 */

/**
 * Carrega a fonte da interface, chamar após InitWindow
 */
void loadResources();

/**
 * Libera a fonte da interface
 */
void unloadResources();

/**
 * Fontes indexadas pelo fontId do Clay
 */
Font* getFonts();

/**
 * Fonte padrão da interface
 */
const Font& getFont();

/**
 * Carrega a imagem embutida com o nome informado, ex: "neuron"
 */
Texture2D loadTextureResource(const char* name);

#endif /* _RESOURCES_H_ */
