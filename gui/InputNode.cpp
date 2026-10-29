#include "InputNode.h"
#include "Widgets.h"

InputNode::InputNode(Texture2D img): Node(img)
{
	valueBox = Rectangle{ 0, 0, 0, 0 };
	bias = false;
}

bool InputNode::IsIn(const Point& p)
{
	int mw = 20 / 2;
	int mh = 20 / 2;
	return p.x >= getPosition().x - mw && p.x <= getPosition().x + mw &&
		   p.y >= getPosition().y - mh && p.y <= getPosition().y + mh;
}

void InputNode::draw()
{
	if(isSelected())
		DrawRectangle(getPosition().x - 14, getPosition().y - 14, 28, 28, ORANGE);
	Node::draw();
	if(!bias)
		return;
	// à esquerda da entrada, cinza por não ser clicável
	std::string text = "bias = 1";
	Vector2 size = measureLabel(text, ui::FontSize);
	Rectangle box = { getPosition().x - 24 - size.x, getPosition().y - size.y / 2, size.x,
		size.y };
	drawLabel(box, text, ui::FontSize, ui::toColor(ui::WindowColor),
		ui::toColor(ui::BorderColor), ui::toColor(ui::StatusTextColor));
}

void InputNode::drawValue()
{
	// o bias já mostra o valor em draw
	valueBox = Rectangle{ 0, 0, 0, 0 };
	if(bias)
		return;
	// à esquerda da entrada, azul quando ligada
	std::string text = formatNumber(getValue(), 2);
	Vector2 size = measureLabel(text, ui::FontSize);
	if(size.x < 26)
		size.x = 26;
	valueBox = Rectangle{ getPosition().x - 24 - size.x, getPosition().y - size.y / 2, size.x,
		size.y };
	if(getValue() != 0)
		drawLabel(valueBox, text, ui::FontSize, ui::toColor(ui::FocusColor),
			ui::toColor(ui::FocusColor), WHITE);
	else
		drawLabel(valueBox, text, ui::FontSize, WHITE, ui::toColor(ui::BorderColor),
			ui::toColor(ui::TextColor));
}

bool InputNode::IsInValue(const Point& p)
{
	return CheckCollisionPointRec(Vector2{ (float)p.x, (float)p.y }, valueBox);
}
