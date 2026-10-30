#include "Icons.h"
#include "Shapes.h"
#include <list>
#include <vector>

namespace icons
{
	/**
	 * Converte pontos da caixa de referência (size) para os limites informados
	 */
	class Canvas
	{
	private:
		Rectangle bounds;
		float scale;
		float stroke;
		Color color;
	public:
		Canvas(Rectangle bounds, Vector2 size, float stroke, Color color): bounds(bounds),
			color(color)
		{
			float sx = bounds.width / size.x, sy = bounds.height / size.y;
			scale = sx < sy ? sx : sy;
			// centraliza a caixa de referência
			this->bounds.x += (bounds.width - size.x * scale) / 2;
			this->bounds.y += (bounds.height - size.y * scale) / 2;
			this->stroke = stroke * scale;
		}
		Vector2 at(float x, float y)
		{
			return Vector2{ bounds.x + x * scale, bounds.y + y * scale };
		}
		void lines(std::vector<Vector2> points)
		{
			for(Vector2& point : points)
				point = at(point.x, point.y);
			shapes::polyline(points.data(), (int)points.size(), stroke, color);
		}
		/**
		 * Curva de Bézier cúbica
		 */
		void curve(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3)
		{
			std::vector<Vector2> points;
			for(int i = 0; i <= 16; i++)
			{
				float t = i / 16.0f, u = 1 - t;
				points.push_back(Vector2{
					u * u * u * p0.x + 3 * u * u * t * p1.x + 3 * u * t * t * p2.x + t * t * t * p3.x,
					u * u * u * p0.y + 3 * u * u * t * p1.y + 3 * u * t * t * p2.y + t * t * t * p3.y });
			}
			lines(points);
		}
		void circle(float x, float y, float radius)
		{
			shapes::strokeCircle(at(x, y), radius * scale, stroke, color);
		}
		void dot(float x, float y, float radius)
		{
			shapes::fillCircle(at(x, y), radius * scale, color);
		}
		void box(float x, float y, float width, float height, float radius, bool fill)
		{
			Vector2 corner = at(x, y);
			Rectangle rect = { corner.x, corner.y, width * scale, height * scale };
			if(fill)
				shapes::fillRoundedRect(rect, radius * scale, color);
			else
			{
				// o contorno do SVG fica centrado na borda
				float half = stroke / 2;
				rect = Rectangle{ rect.x - half, rect.y - half, rect.width + stroke,
					rect.height + stroke };
				shapes::strokeRoundedRect(rect, radius * scale + half, stroke, color);
			}
		}
		void polygon(std::vector<Vector2> points)
		{
			for(Vector2& point : points)
				point = at(point.x, point.y);
			shapes::fillPolygon(points.data(), (int)points.size(), color);
		}
	};

	void draw(Icon icon, Rectangle bounds, Color color)
	{
		Vector2 size = { 18, 18 };
		if(icon == Arrow)
			size = Vector2{ 16, 16 };
		else if(icon == InputData || icon == InputBias || icon == Trash)
			size = Vector2{ 30, 20 };
		else if(icon == Check)
			size = Vector2{ 14, 14 };
		float stroke = icon == Check ? 2 : icon == Trash ? 1.6f : 1.8f;
		if(icon == InputData || icon == InputBias)
			stroke = 2;
		Canvas c(bounds, size, stroke, color);
		switch(icon)
		{
		case Neuron:
			c.circle(9, 9, 7);
			c.lines({ { 5.5f, 11 }, { 9, 11 }, { 9, 7 }, { 12.5f, 7 } });
			break;
		case Input:
			c.box(2.5f, 2.5f, 13, 13, 3.5f, false);
			c.lines({ { 6, 9 }, { 12, 9 } });
			c.lines({ { 9.5f, 6.5f }, { 12, 9 }, { 9.5f, 11.5f } });
			break;
		case Output:
			c.circle(9, 9, 7);
			c.dot(9, 9, 2.5f);
			break;
		case Play:
			c.polygon({ { 5.5f, 3.8f }, { 14.5f, 9 }, { 5.5f, 14.2f } });
			break;
		case Stop:
			c.box(4.5f, 4.5f, 9, 9, 1.5f, true);
			break;
		case Train:
			c.lines({ { 2.5f, 5 }, { 7.5f, 10 }, { 10.5f, 7 }, { 15.5f, 12 } });
			c.lines({ { 11.5f, 12 }, { 15.5f, 12 }, { 15.5f, 8 } });
			break;
		case Save:
			c.lines({ { 9, 2.5f }, { 9, 11 } });
			c.lines({ { 5.5f, 7.5f }, { 9, 11 }, { 12.5f, 7.5f } });
			c.lines({ { 3, 12 }, { 3, 14 }, { 4.5f, 15.5f }, { 13.5f, 15.5f }, { 15, 14 },
				{ 15, 12 } });
			break;
		case Arrange:
		{
			// rede em camadas: dois nós, dois nós e um
			Canvas thin(bounds, size, 1.2f, color);
			thin.lines({ { 3.5f, 5 }, { 9, 5 }, { 14.5f, 9 }, { 9, 13 }, { 3.5f, 13 } });
			c.dot(3.5f, 5, 2);
			c.dot(3.5f, 13, 2);
			c.dot(9, 5, 2);
			c.dot(9, 13, 2);
			c.dot(14.5f, 9, 2);
			break;
		}
		case Dice:
			c.box(2.5f, 2.5f, 13, 13, 3.5f, false);
			c.dot(6, 6, 1.4f);
			c.dot(9, 9, 1.4f);
			c.dot(12, 12, 1.4f);
			break;
		case Arrow:
			c.lines({ { 3, 8 }, { 12, 8 } });
			c.lines({ { 8.5f, 4.5f }, { 12, 8 }, { 8.5f, 11.5f } });
			break;
		case InputData:
			c.lines({ { 4, 16 }, { 14, 16 }, { 14, 4 }, { 26, 4 } });
			break;
		case InputBias:
			c.lines({ { 3, 4 }, { 27, 4 } });
			break;
		case Trash:
			c.lines({ { 9.5f, 6 }, { 20.5f, 6 } });
			c.lines({ { 13, 6 }, { 13, 4.5f }, { 17, 4.5f }, { 17, 6 } });
			c.lines({ { 11, 6 }, { 11.8f, 16 }, { 18.2f, 16 }, { 19, 6 } });
			break;
		case Check:
			c.lines({ { 2.5f, 7.5f }, { 5.5f, 10.5f }, { 11.5f, 3.5f } });
			break;
		}
	}

	void drawFunction(int function, Rectangle bounds, Color color)
	{
		Canvas axis(bounds, Vector2{ 30, 20 }, 1, Color{ 226, 232, 240, 255 });
		axis.lines({ { 15, 1 }, { 15, 19 } });
		axis.lines({ { 1, 10 }, { 29, 10 } });
		Canvas c(bounds, Vector2{ 30, 20 }, 2, color);
		// mesma ordem de NeuronNode::FunctionType
		switch(function)
		{
		case 0:
			c.lines({ { 3, 17 }, { 27, 3 } });
			break;
		case 1:
			c.lines({ { 3, 15 }, { 15, 15 }, { 15, 5 }, { 27, 5 } });
			break;
		case 2:
			c.lines({ { 3, 17 }, { 15, 17 }, { 15, 3 }, { 27, 3 } });
			break;
		case 3:
			c.curve({ 2, 16 }, { 12, 16 }, { 18, 4 }, { 28, 4 });
			break;
		default:
			c.curve({ 2, 18 }, { 12, 18 }, { 18, 2 }, { 28, 2 });
			break;
		}
	}

	struct IconDraw
	{
		CustomDraw custom;
		int icon;
		int function;
		Color color;
	};

	// endereços estáveis, o Clay guarda o ponteiro até desenhar o quadro
	static std::list<IconDraw> draws;

	static void drawCustom(void* context, Rectangle bounds)
	{
		IconDraw* item = (IconDraw*)context;
		if(item->function >= 0)
			drawFunction(item->function, bounds, item->color);
		else
			draw((Icon)item->icon, bounds, item->color);
	}

	static CustomDraw* find(int icon, int function, Color color)
	{
		for(IconDraw& item : draws)
		{
			if(item.icon == icon && item.function == function && item.color.r == color.r &&
				item.color.g == color.g && item.color.b == color.b && item.color.a == color.a)
				return &item.custom;
		}
		draws.push_back(IconDraw{ { drawCustom, NULL }, icon, function, color });
		draws.back().custom.context = &draws.back();
		return &draws.back().custom;
	}

	CustomDraw* custom(Icon icon, Color color)
	{
		return find(icon, -1, color);
	}

	CustomDraw* customFunction(int function)
	{
		return find(-1, function, Color{ 51, 65, 85, 255 });
	}
}
