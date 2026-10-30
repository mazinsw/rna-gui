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
 * Inicializa o Clay e registra a medição de texto com a fonte da interface
 */
void ClayRaylib_Initialize();

/**
 * Libera a memória do Clay
 */
void ClayRaylib_Shutdown();

/**
 * Desenha os comandos gerados pelo layout
 */
void ClayRaylib_Render(Clay_RenderCommandArray commands);

/**
 * Tamanho da interface, como o zoom de um navegador: o layout fica em unidades
 * da interface, multiplicadas pela escala ao desenhar e ao medir o texto
 */
void ClayRaylib_SetScale(float scale);
float ClayRaylib_GetScale();

/**
 * Tamanho da janela em unidades da interface, para o layout
 */
Clay_Dimensions ClayRaylib_GetLayoutDimensions();

/**
 * Ponto da janela em unidades da interface
 */
Vector2 ClayRaylib_ToLayout(Vector2 point);

/**
 * Largura do texto em unidades da interface, a mesma medida pelo layout
 */
float ClayRaylib_MeasureText(const std::string& text, uint16_t fontSize);

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
