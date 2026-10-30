#ifndef _NODE_H_
#define _NODE_H_
#include <raylib.h>
#include <math.h>
#include <string>
#include "Number.h"

struct Point
{
	int x, y;
	Point(int x = 0, int y = 0): x(x), y(y) {}
};

/**
 * classe responsável por desenhar o neurônio
 */
class Node
{
private:
	Point p;
	bool selected;
	double value;
public:
	/**
	 * Tipo do nó, mesmos valores gravados no arquivo
	 */
	enum Type
	{
		TypeNeuron = 1,
		TypeInput,
		TypeOutput
	};
public:
	Node();
	virtual ~Node() {}

	inline void setSelected(bool selected)
	{
		this->selected = selected;
	}

	inline bool isSelected()
	{
		return selected;
	}

	inline void setPosition(int x, int y)
	{
		this->p.x = x;
		this->p.y = y;
	}

	inline void setPosition(const Point& p)
	{
		setPosition(p.x, p.y);
	}

	inline const Point& getPosition()
	{
		return p;
	}

	inline Vector2 getCenter()
	{
		return Vector2{ (float)p.x, (float)p.y };
	}

	/**
	 * Valor do nó na execução: o informado na entrada ou o calculado no
	 * neurônio e na saída
	 */
	inline void setValue(double value)
	{
		this->value = value;
	}

	inline double getValue()
	{
		return value;
	}

	inline bool is(Type type)
	{
		return getType() == type;
	}

	virtual Type getType() = 0;

	/**
	 * Desenha o nó, com os valores quando a rede está em execução
	 */
	virtual void draw(bool running) = 0;

	virtual bool IsIn(const Point& p) = 0;

	/**
	 * Indica se o ponto está sobre um valor clicável desenhado na execução
	 */
	virtual bool IsInValue(const Point& p)
	{
		return false;
	}

	/**
	 * Veja number::format
	 */
	static inline std::string formatNumber(double value, int digits)
	{
		return number::format(value, digits);
	}

	/**
	 * Veja number::parse
	 */
	static inline bool parseNumber(const std::string& text, double& value)
	{
		return number::parse(text, value);
	}

	/**
	 * Zoom e canto da origem da rede na janela durante o desenho, para o texto
	 * ser rasterizado no tamanho da tela e alinhado aos pixels dela
	 */
	static void setView(float zoom, Vector2 origin);

	static Vector2 measureText(const std::string& text, float fontSize);

	/**
	 * Texto com o canto superior esquerdo na posição
	 */
	static void drawText(const std::string& text, Vector2 position, float fontSize, Color color);
	static void drawTextCentered(const std::string& text, Vector2 center, float fontSize,
		Color color);

	/**
	 * Caixa com as pontas arredondadas, fundo dos rótulos de peso, bias e valor
	 */
	static void drawPill(Rectangle box, Color fill, Color border, float borderWidth = 1);

	/**
	 * Caixa do rótulo com o texto centralizado na posição
	 */
	static Rectangle pillBox(Vector2 center, const std::string& text, float fontSize,
		float height);
};

#endif
