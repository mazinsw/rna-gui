#ifndef _RESOURCES_H_
#define _RESOURCES_H_
#include <raylib.h>

/**
 * Fonte da interface embutida no executável, substitui o Resources.rc da
 * versão Windows
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

#endif /* _RESOURCES_H_ */
