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

std::string Node::formatNumber(double value, int digits)
{
	if(std::isnan(value))
		return "nan";
	if(std::isinf(value))
		return value > 0 ? "inf" : "-inf";
	char str[64];
	// os grandes em notação científica para caber no rótulo: 1,23e15
	snprintf(str, sizeof(str), fabs(value) >= 1e6 ? "%.*e" : "%.*f", digits, value);
	std::string s(str), exponent;
	size_t e = s.find('e');
	if(e != std::string::npos)
	{
		exponent = "e" + std::to_string(atoi(s.c_str() + e + 1));
		s.resize(e);
	}
	if(s.find('.') != std::string::npos)
	{
		size_t c = s.size();
		while(s[c - 1] == '0')
			c--;
		if(s[c - 1] == '.')
			c--;
		s.resize(c);
	}
	if(s == "-0")
		s = "0";
	size_t dot = s.find('.');
	if(dot != std::string::npos)
		s[dot] = ',';
	return s + exponent;
}

bool Node::parseNumber(const std::string& text, double& value)
{
	std::string str = text;
	std::replace(str.begin(), str.end(), ',', '.');
	char* end;
	double result = strtod(str.c_str(), &end);
	// strtod também aceita "nan" e "inf", que estragariam a rede
	if(end == str.c_str() || !std::isfinite(result))
		return false;
	value = result;
	return true;
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
