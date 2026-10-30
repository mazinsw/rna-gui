#include "Edge.h"
#include "Shapes.h"
#include "Widgets.h"
#include <math.h>

Edge::Edge(Node *first, Node *second, double weight)
{
	influence = 0;
	thickness = 1.5f;
	labelBox = Rectangle{ 0, 0, 0, 0 };
	setFirst(first);
	setSecond(second);
	setWeight(weight);
	setSelected(false);
	setChanged(false);
}

Color Edge::influenceColor(float influence)
{
	static const struct
	{
		float position;
		Color color;
	} stops[] = {
		{ 0, { 203, 213, 225, 255 } },
		{ 0.35f, { 251, 191, 36, 255 } },
		{ 0.7f, { 249, 115, 22, 255 } },
		{ 1, { 220, 38, 38, 255 } },
	};
	for(int i = 1; i < 4; i++)
	{
		if(influence > stops[i].position && i < 3)
			continue;
		float t = (influence - stops[i - 1].position) / (stops[i].position - stops[i - 1].position);
		t = t < 0 ? 0 : t > 1 ? 1 : t;
		const Color& a = stops[i - 1].color;
		const Color& b = stops[i].color;
		return Color{ (unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
			(unsigned char)(a.b + (b.b - a.b) * t), 255 };
	}
	return stops[3].color;
}

void Edge::drawLine()
{
	thickness = 1.5f + 3 * influence;
	Color color = influenceColor(influence);
	if(isSelected())
	{
		color = ui::toColor(ui::AccentColor);
		if(thickness < 2.5f)
			thickness = 2.5f;
	}
	shapes::line(first->getCenter(), second->getCenter(), thickness, color);
}

void Edge::drawLabel()
{
	Vector2 a = first->getCenter(), b = second->getCenter();
	Vector2 middle = { (a.x + b.x) / 2, (a.y + b.y) / 2 };
	labelBox = Node::pillBox(middle, weightStr, ui::LabelFontSize, 22);
	if(isChanged())
		Node::drawPill(labelBox, ui::toColor(ui::ChangedColor),
			ui::toColor(ui::ChangedBorderColor));
	else if(isSelected())
		Node::drawPill(labelBox, ui::toColor(ui::AccentLightColor), ui::toColor(ui::AccentColor),
			1.5f);
	else
		Node::drawPill(labelBox, WHITE, ui::toColor(ui::PillBorderColor));
	Node::drawTextCentered(weightStr, middle, ui::LabelFontSize, ui::toColor(ui::InkColor));
}

void Edge::hideLabel()
{
	labelBox = Rectangle{ 0, 0, 0, 0 };
}

bool Edge::IsInLabel(const Point& p)
{
	return CheckCollisionPointRec(Vector2{ (float)p.x, (float)p.y }, labelBox);
}

bool Edge::IsIn(const Point& p)
{
	if(IsInLabel(p))
		return true;
	// distância do ponto ao segmento
	Vector2 a = first->getCenter(), b = second->getCenter();
	float dx = b.x - a.x, dy = b.y - a.y;
	float length = dx * dx + dy * dy;
	float t = length > 0 ? ((p.x - a.x) * dx + (p.y - a.y) * dy) / length : 0;
	t = t < 0 ? 0 : t > 1 ? 1 : t;
	float x = a.x + t * dx - p.x, y = a.y + t * dy - p.y;
	float tolerance = thickness / 2 + 2;
	if(tolerance < 4)
		tolerance = 4;
	return sqrtf(x * x + y * y) <= tolerance;
}
