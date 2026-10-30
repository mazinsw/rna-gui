#include "Node.h"
#include "Resources.h"
#include "Shapes.h"
#include <algorithm>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>

static float viewZoom = 1;
static Vector2 viewOrigin = { 0, 0 };

Node::Node()
{
	setSelected(false);
	setValue(0);
}

void Node::setView(float zoom, Vector2 origin)
{
	viewZoom = zoom;
	viewOrigin = origin;
}

Vector2 Node::measureText(const std::string& text, float fontSize)
{
	return MeasureTextEx(getFont(fontSize * viewZoom), text.c_str(), fontSize, 0);
}

/**
 * Coordenada da rede no pixel da tela mais próximo
 */
static float toPixel(float value, float origin)
{
	float scale = getRenderScale();
	return (roundf((value * viewZoom + origin) * scale) / scale - origin) / viewZoom;
}

void Node::drawText(const std::string& text, Vector2 position, float fontSize, Color color)
{
	// no pixel da tela, para não borrar o texto
	DrawTextEx(getFont(fontSize * viewZoom), text.c_str(), Vector2{
		toPixel(position.x, viewOrigin.x), toPixel(position.y, viewOrigin.y) }, fontSize, 0,
		color);
}

void Node::drawTextCentered(const std::string& text, Vector2 center, float fontSize,
	Color color)
{
	Vector2 size = measureText(text, fontSize);
	drawText(text, Vector2{ center.x - size.x / 2, center.y - size.y / 2 }, fontSize, color);
}

void Node::drawPill(Rectangle box, Color fill, Color border, float borderWidth)
{
	shapes::fillRoundedRect(box, box.height / 2, fill);
	shapes::strokeRoundedRect(box, box.height / 2, borderWidth, border);
}

Rectangle Node::pillBox(Vector2 center, const std::string& text, float fontSize, float height)
{
	float width = measureText(text, fontSize).x + 16;
	return Rectangle{ center.x - width / 2, center.y - height / 2, width, height };
}
