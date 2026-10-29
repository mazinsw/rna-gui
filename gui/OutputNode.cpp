#include "OutputNode.h"
#include "Widgets.h"

OutputNode::OutputNode(Texture2D img): Node(img)
{
	target = 0;
	targetBox = Rectangle{ 0, 0, 0, 0 };
}

bool OutputNode::IsIn(const Point& p)
{
	int mw = 20 / 2;
	int mh = 20 / 2;
	return p.x >= getPosition().x - mw && p.x <= getPosition().x + mw &&
		   p.y >= getPosition().y - mh && p.y <= getPosition().y + mh;
}

void OutputNode::draw()
{
	if(isSelected())
		DrawCircle(getPosition().x, getPosition().y, 14, ORANGE);
	Node::draw();
}

void OutputNode::drawValue()
{
	// abaixo da saída: o valor calculado, verde quando igual ao alvo, e o alvo
	std::string value = "saída " + formatNumber(getValue(), 2);
	std::string goal = "alvo " + formatNumber(target, 2);
	Vector2 valueSize = measureLabel(value, ui::FontSize);
	Vector2 goalSize = measureLabel(goal, ui::FontSize);
	float x = getPosition().x - (valueSize.x + 6 + goalSize.x) / 2;
	float y = getPosition().y + 20;
	Color state = ui::toColor(isOnTarget() ? ui::SuccessColor : ui::WarningColor);
	drawLabel(Rectangle{ x, y, valueSize.x, valueSize.y }, value, ui::FontSize, WHITE, state,
		state);
	targetBox = Rectangle{ x + valueSize.x + 6, y, goalSize.x, goalSize.y };
	if(target != 0)
		drawLabel(targetBox, goal, ui::FontSize, ui::toColor(ui::FocusColor),
			ui::toColor(ui::FocusColor), WHITE);
	else
		drawLabel(targetBox, goal, ui::FontSize, WHITE, ui::toColor(ui::BorderColor),
			ui::toColor(ui::TextColor));
}

bool OutputNode::IsInValue(const Point& p)
{
	return CheckCollisionPointRec(Vector2{ (float)p.x, (float)p.y }, targetBox);
}
