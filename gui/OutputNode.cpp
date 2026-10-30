#include "OutputNode.h"
#include "Shapes.h"
#include "Widgets.h"
#include <cmath>

OutputNode::OutputNode()
{
	target = 0;
	targetBox = Rectangle{ 0, 0, 0, 0 };
}

bool OutputNode::IsIn(const Point& p)
{
	int dx = p.x - getPosition().x, dy = p.y - getPosition().y;
	return dx * dx + dy * dy <= 18 * 18;
}

void OutputNode::draw(bool running)
{
	// alvo: verde quando o valor é igual ao alvo, laranja quando difere e
	// vermelho quando o valor estourou
	Vector2 center = getCenter();
	Color color = ui::toColor(ui::NodeColor);
	if(running && !std::isfinite(getValue()))
		color = ui::toColor(ui::DangerColor);
	else if(running)
		color = ui::toColor(isOnTarget() ? ui::SuccessColor : ui::WarningColor);
	if(isSelected())
		shapes::strokeCircle(center, 21, 4, ui::toColor(ui::SelectionRingColor));
	shapes::fillCircle(center, 18, WHITE);
	shapes::strokeCircle(center, 16.5f, 3, color);
	shapes::fillCircle(center, 6, color);
	targetBox = Rectangle{ 0, 0, 0, 0 };
	if(!running)
		return;
	// abaixo da saída: o valor calculado e o alvo, que alterna ao clicar
	std::string value = "saída " + formatNumber(getValue(), 2);
	std::string goal = "alvo " + formatNumber(target, 2);
	float size = ui::LabelFontSize;
	float valueWidth = measureText(value, size).x + 18;
	float goalWidth = measureText(goal, size).x + 18;
	float x = center.x - (valueWidth + 6 + goalWidth) / 2;
	float y = center.y + 26;
	Rectangle valueBox = { x, y, valueWidth, 24 };
	drawPill(valueBox, WHITE, color);
	drawTextCentered(value, Vector2{ valueBox.x + valueBox.width / 2, y + 12 }, size, color);
	targetBox = Rectangle{ x + valueWidth + 6, y, goalWidth, 24 };
	if(target != 0)
		drawPill(targetBox, ui::toColor(ui::AccentColor), ui::toColor(ui::AccentColor));
	else
		drawPill(targetBox, WHITE, ui::toColor(ui::PillBorderColor));
	drawTextCentered(goal, Vector2{ targetBox.x + targetBox.width / 2, y + 12 }, size,
		target != 0 ? WHITE : ui::toColor(ui::InkColor));
}

bool OutputNode::IsInValue(const Point& p)
{
	return CheckCollisionPointRec(Vector2{ (float)p.x, (float)p.y }, targetBox);
}
