#ifndef _ICONS_H_
#define _ICONS_H_
#include <raylib.h>
#include "ClayRaylib.h"

/**
 * Ícones desenhados com linhas suavizadas, sem imagens
 */
namespace icons
{
	enum Icon
	{
		// painel de botões, em uma caixa 18x18
		Neuron,
		Input,
		Output,
		Play,
		Stop,
		Train,
		Save,
		// dentro da entrada, 16x16
		Arrow,
		// menu, em uma caixa 30x20
		InputData,
		InputBias,
		Trash,
		// marca do item atual do menu, 14x14
		Check
	};

	void draw(Icon icon, Rectangle bounds, Color color);

	/**
	 * Gráfico da função de ativação com os eixos, em uma caixa 30x20
	 */
	void drawFunction(int function, Rectangle bounds, Color color);

	/**
	 * Elemento custom do Clay que desenha o ícone, válido até o fim do programa
	 */
	CustomDraw* custom(Icon icon, Color color);
	CustomDraw* customFunction(int function);
}

#endif /* _ICONS_H_ */
