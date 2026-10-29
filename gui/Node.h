#ifndef _NODE_H_
#define _NODE_H_
#include <raylib.h>
#include <math.h>

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

	inline bool is(Type type)
	{
		return getType() == type;
	}

	virtual Type getType() = 0;

	virtual void draw();

	virtual bool IsIn(const Point& p) = 0;
};

#endif
