#ifndef _SHAPES_H_
#define _SHAPES_H_
#include <raylib.h>

/**
 * Formas com a borda suavizada: cada contorno ganha uma faixa de 1 pixel da
 * tela que vai da cor ao transparente, sem depender do MSAA
 */
namespace shapes
{
	/**
	 * Atualiza a largura da borda suavizada com a escala da tela, chamar a cada
	 * quadro
	 */
	void beginFrame();

	void line(Vector2 a, Vector2 b, float thickness, Color color);

	/**
	 * Linha por vários pontos, com juntas e pontas arredondadas
	 */
	void polyline(const Vector2* points, int count, float thickness, Color color);

	/**
	 * Polígono convexo
	 */
	void fillPolygon(const Vector2* points, int count, Color color);
	void fillCircle(Vector2 center, float radius, Color color);

	/**
	 * Contorno centrado no raio informado
	 */
	void strokeCircle(Vector2 center, float radius, float thickness, Color color);
	void fillRoundedRect(Rectangle rect, float radius, Color color);

	/**
	 * Contorno por dentro do retângulo
	 */
	void strokeRoundedRect(Rectangle rect, float radius, float thickness, Color color);
}

#endif /* _SHAPES_H_ */
