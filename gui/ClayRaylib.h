#ifndef _CLAYRAYLIB_H_
#define _CLAYRAYLIB_H_
#include <raylib.h>
#include <clay.h>
#include <string>

/**
 * Desenho próprio de um elemento custom do Clay, informado em
 * .custom = { .customData = &customDraw }
 */
struct CustomDraw
{
	void (*draw)(void* context, Rectangle bounds);
	void* context;
};

/**
 * Inicializa o Clay e registra a medição de texto com as fontes informadas
 */
void ClayRaylib_Initialize(Font* fonts);

/**
 * Libera a memória do Clay
 */
void ClayRaylib_Shutdown();

/**
 * Desenha os comandos gerados pelo layout
 */
void ClayRaylib_Render(Clay_RenderCommandArray commands, Font* fonts);

/**
 * Converte uma string que deve continuar viva até o fim do quadro
 */
inline Clay_String toClayString(const std::string& str)
{
	Clay_String result;
	result.isStaticallyAllocated = false;
	result.length = (int32_t)str.size();
	result.chars = str.c_str();
	return result;
}

#endif /* _CLAYRAYLIB_H_ */
