#include "Node.h"
#include "Resources.h"
#include "Shapes.h"
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

Node::Node()
{
	setSelected(false);
	setValue(0);
}

std::string Node::formatNumber(double value, int digits)
{
	char str[64];
	snprintf(str, sizeof(str), "%.*f", digits, value);
	std::string s(str);
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
	return s;
}

bool Node::parseNumber(const std::string& text, double& value)
{
	std::string str = text;
	std::replace(str.begin(), str.end(), ',', '.');
	char* end;
	double result = strtod(str.c_str(), &end);
	if(end == str.c_str())
		return false;
	value = result;
	return true;
}

Vector2 Node::measureText(const std::string& text, float fontSize)
{
	return MeasureTextEx(getFont(fontSize), text.c_str(), fontSize, 0);
}

void Node::drawText(const std::string& text, Vector2 position, float fontSize, Color color)
{
	// no pixel da tela, para não borrar o texto
	float scale = getRenderScale();
	DrawTextEx(getFont(fontSize), text.c_str(), Vector2{ roundf(position.x * scale) / scale,
		roundf(position.y * scale) / scale }, fontSize, 0, color);
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
