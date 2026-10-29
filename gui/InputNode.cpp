#include "InputNode.h"
#include "Icons.h"
#include "Shapes.h"
#include "Widgets.h"
#include <stdlib.h>

InputNode::InputNode()
{
	bias = false;
}

bool InputNode::IsIn(const Point& p)
{
	return abs(p.x - getPosition().x) <= 17 && abs(p.y - getPosition().y) <= 17;
}

void InputNode::draw(bool running)
{
	// quadrado com o valor dentro durante a execução
	Vector2 center = getCenter();
	Rectangle box = { center.x - 17, center.y - 17, 34, 34 };
	if(isSelected())
		shapes::strokeRoundedRect(Rectangle{ box.x - 5, box.y - 5, box.width + 10,
			box.height + 10 }, 14, 4, ui::toColor(ui::SelectionRingColor));
	Color fill = ui::toColor(ui::AccentLightColor);
	Color border = ui::toColor(ui::AccentColor);
	Color text = ui::toColor(ui::AccentDarkColor);
	if(bias)
	{
		fill = ui::toColor(ui::HoverColor);
		border = ui::toColor(ui::FaintColor);
		text = ui::toColor(ui::StatusTextColor);
	}
	else if(running && getValue() != 0)
	{
		fill = ui::toColor(ui::AccentColor);
		text = WHITE;
	}
	else if(running)
		fill = WHITE;
	shapes::fillRoundedRect(box, 9, fill);
	shapes::strokeRoundedRect(box, 9, 2, border);
	if(bias || running)
		drawTextCentered(formatNumber(getValue(), 2), center, 15, text);
	else
		icons::draw(icons::Arrow, Rectangle{ center.x - 8, center.y - 8, 16, 16 }, border);
	if(bias)
		drawTextCentered("bias", Vector2{ center.x, center.y + 27 }, 11,
			ui::toColor(ui::MutedColor));
}
