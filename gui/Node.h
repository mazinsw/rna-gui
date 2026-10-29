#ifndef _NODE_H_
#define _NODE_H_
#include <raylib.h>
#include <math.h>
#include <string>

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
	Texture2D img;
	bool selected;
	double value;
protected:
	/**
	 * Tamanho da caixa de drawLabel para o texto informado
	 */
	static Vector2 measureLabel(const std::string& text, float fontSize);

	/**
	 * Desenha o texto centralizado numa caixa de cantos arredondados
	 */
	static void drawLabel(Rectangle box, const std::string& text, float fontSize, Color fill,
		Color border, Color color);
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
	Node(Texture2D img);
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

	inline Texture2D getImage()
	{
		return img;
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

	virtual void draw();

	virtual bool IsIn(const Point& p) = 0;

	/**
	 * Desenha o valor do nó durante a execução
	 */
	virtual void drawValue() {}

	/**
	 * Indica se o ponto está sobre um valor clicável desenhado por drawValue
	 */
	virtual bool IsInValue(const Point& p)
	{
		return false;
	}

	/**
	 * Número com vírgula decimal, no máximo digits casas e sem zeros à direita
	 */
	static std::string formatNumber(double value, int digits);

	/**
	 * Lê um número com vírgula ou ponto decimal, retorna false se o texto não
	 * começa com um número
	 */
	static bool parseNumber(const std::string& text, double& value);
};

#endif
