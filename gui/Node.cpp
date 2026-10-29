#include "Node.h"
#include "Resources.h"
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

Node::Node(Texture2D img)
{
	this->img = img;
	setSelected(false);
	setValue(0);
}

void Node::draw()
{
	DrawTexture(img, p.x - img.width / 2, p.y - img.height / 2, WHITE);
}

Vector2 Node::measureLabel(const std::string& text, float fontSize)
{
	Vector2 size = MeasureTextEx(getFont(), text.c_str(), fontSize, 0);
	return Vector2{ size.x + 16, fontSize + 9 };
}

void Node::drawLabel(Rectangle box, const std::string& text, float fontSize, Color fill,
	Color border, Color color)
{
	// cantos com raio 6
	float roundness = 12 / (box.width < box.height ? box.width : box.height);
	Vector2 size = MeasureTextEx(getFont(), text.c_str(), fontSize, 0);
	DrawRectangleRounded(box, roundness, 4, fill);
	DrawRectangleRoundedLinesEx(box, roundness, 4, 1.0f, border);
	DrawTextEx(getFont(), text.c_str(), Vector2{ roundf(box.x + (box.width - size.x) / 2),
		roundf(box.y + (box.height - size.y) / 2) }, fontSize, 0, color);
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
