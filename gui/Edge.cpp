#include "Edge.h"
#include "Resources.h"
#include <math.h>

#define WEIGHT_FONT_SIZE 13

Edge::Edge(Node *first, Node *second, double weight)
{
	sz.x = sz.y = 0;
	setFirst(first);
	setSecond(second);
	setWeight(weight);
	setSelected(false);
	setChanged(false);
}

void Edge::draw()
{
	int x1 = first->getPosition().x;
	int y1 = first->getPosition().y;
	int x2 = second->getPosition().x;
	int y2 = second->getPosition().y;
	Color color = isSelected() ? ORANGE : BLACK;
	DrawLineEx(Vector2{ (float)x1, (float)y1 }, Vector2{ (float)x2, (float)y2 }, 1.0f, color);
	sz = MeasureTextEx(getFont(), weightStr.c_str(), WEIGHT_FONT_SIZE, 0);
	Rectangle box = { (float)((x1 + x2) / 2 - 6), (float)((y1 + y2) / 2 - 6),
		sz.x + 12, sz.y + 12 };
	// cantos com raio 4
	float roundness = 8 / (box.width < box.height ? box.width : box.height);
	if(isChanged())
		color = Color{ 0xFF, 0xE5, 0x8F, 0xFF };
	else if(isSelected())
		color = Color{ 0xF0, 0xF0, 0xF0, 0xFF };
	else
		color = Color{ 0xE0, 0xF0, 0xE0, 0xFF };
	DrawRectangleRounded(box, roundness, 4, color);
	DrawRectangleRoundedLinesEx(box, roundness, 4, 1.0f, Color{ 0, 127, 255, 255 });
	DrawTextEx(getFont(), weightStr.c_str(), Vector2{ (float)((x1 + x2) / 2),
		(float)((y1 + y2) / 2) }, WEIGHT_FONT_SIZE, 0, Color{ 0, 0, 255, 255 });
}

bool Edge::IsIn(const Point& p)
{
	int x1 = first->getPosition().x;
	int y1 = first->getPosition().y;
	int x2 = p.x;
	int y2 = p.y;
	int x3 = second->getPosition().x;
	int y3 = second->getPosition().y;
	double d;
	int dist = (x1 - x3) * (x1 - x3) + (y1 - y3) * (y1 - y3);
	if (dist == 0)
	{
		dist = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
		d = sqrt(dist);
	}
	else
	{
		double t = ((x2 - x1) * (x3 - x1) + (y2 - y1) * (y3 - y1))
		           / (double) dist;
		if (t < 0)
		{
			dist = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
			d = sqrt(dist);
		}
		else if (t > 1)
		{
			dist = (x2 - x3) * (x2 - x3) + (y2 - y3) * (y2 - y3);
			d = sqrt(dist);
		}
		else
		{
			double x = x1 + t * (x3 - x1);
			double y = y1 + t * (y3 - y1);
			d = sqrt((x2 - x) * (x2 - x) + (y2 - y) * (y2 - y));
		}
	}
	if (fabs(d) < 4)
		return true;
	int mw = (int)(sz.x + 12) / 2;
	int mh = (int)(sz.y + 12) / 2;
	x1 = (x1 + x3) / 2 + mw;
	y1 = (y1 + y3) / 2 + mh;
	return x2 >= x1 - mw && x2 <= x1 + mw &&
		   y2 >= y1 - mh && y2 <= y1 + mh;
}
