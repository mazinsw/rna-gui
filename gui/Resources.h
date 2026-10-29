#ifndef _RESOURCES_H_
#define _RESOURCES_H_
#include <raylib.h>

/**
 * Fonte da interface embutida no executável, substitui o Resources.rc da
 * versão Windows
 */

/**
 * Confere a fonte da interface, chamar após InitWindow
 */
void loadResources();

/**
 * Libera as fontes carregadas
 */
void unloadResources();

/**
 * Pixels da tela por unidade de coordenada da janela: 2 no Retina, 1 nas
 * demais telas
 */
float getRenderScale();

/**
 * Fonte rasterizada no tamanho informado em pixels da tela, para o texto
 * ficar nítido sem ser reduzido
 */
const Font& getFont(float size);

#endif /* _RESOURCES_H_ */
