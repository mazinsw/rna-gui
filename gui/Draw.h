#ifndef _DRAW_H_
#define _DRAW_H_
#include <raylib.h>
#include <string>

/**
 * Partes da rede desenhadas na área de edição, em coordenadas da rede: os
 * nós, os rótulos e os textos
 */
namespace draw
{
	// raio do neurônio e da saída e meio lado da entrada
	const float NeuronRadius = 28;
	const float OutputRadius = 18;
	const float InputHalf = 17;

	/**
	 * Zoom e canto da origem da rede na janela durante o desenho, para o texto
	 * ser rasterizado no tamanho da tela e alinhado aos pixels dela
	 */
	void setView(float zoom, Vector2 origin);

	Vector2 measureText(const std::string& text, float fontSize);

	/**
	 * Texto com o canto superior esquerdo na posição
	 */
	void text(const std::string& text, Vector2 position, float fontSize, Color color);
	void textCentered(const std::string& text, Vector2 center, float fontSize, Color color);

	/**
	 * Caixa com as pontas arredondadas, fundo dos rótulos de peso, bias e valor
	 */
	void pill(Rectangle box, Color fill, Color border, float borderWidth = 1);

	/**
	 * Caixa do rótulo com o texto centralizado na posição
	 */
	Rectangle pillBox(Vector2 center, const std::string& text, float fontSize, float height);

	/**
	 * Do cinza ao vermelho, passando por âmbar e laranja, da influência 0 a 1
	 */
	Color influenceColor(float influence);

	/**
	 * Entrada: quadrado com o valor dentro durante a execução, na cor da
	 * intensidade quando ele está entre 0 e 1
	 */
	void input(Vector2 center, float value, bool running, bool selected);

	/**
	 * Neurônio: círculo com o gráfico da função de ativação
	 */
	void neuron(Vector2 center, int function, bool selected);

	/**
	 * Valor calculado no neurônio, abaixo dele, em vermelho se estourou
	 */
	void neuronValue(Vector2 center, float value);

	/**
	 * Rótulo do peso ou do bias (com prefix antes) centralizado na posição,
	 * retorna a caixa dele
	 */
	Rectangle label(Vector2 center, const std::string& prefix, float value, bool changed,
		bool selected);

	/**
	 * Saída: verde quando o valor é igual ao alvo, laranja quando difere e
	 * vermelho quando estourou; na execução o valor e o alvo abaixo dela.
	 * Retorna a caixa do alvo, vazia fora da execução.
	 */
	Rectangle output(Vector2 center, float value, float target, bool running, bool selected);
}

#endif /* _DRAW_H_ */
