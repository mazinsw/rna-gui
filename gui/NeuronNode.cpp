#include "NeuronNode.h"
#include "Icons.h"
#include "Shapes.h"
#include "Widgets.h"

NeuronNode::NeuronNode()
{
	setFunctionType(Linear);
	setBias(0);
	setBiasChanged(false);
	setBiasSelected(false);
	biasBox = Rectangle{ 0, 0, 0, 0 };
}

bool NeuronNode::IsIn(const Point& p)
{
	// verifica se o ponto está dentro do círculo
	int dx = p.x - getPosition().x, dy = p.y - getPosition().y;
	return dx * dx + dy * dy <= 28 * 28;
}

bool NeuronNode::IsInBias(const Point& p)
{
	return CheckCollisionPointRec(Vector2{ (float)p.x, (float)p.y }, biasBox);
}

void NeuronNode::draw(bool running)
{
	// círculo com o gráfico da função de ativação
	Vector2 center = getCenter();
	Color ink = ui::toColor(ui::NodeColor);
	shapes::fillCircle(Vector2{ center.x, center.y + 2 }, 29, Color{ 15, 23, 42, 30 });
	if(isSelected())
		shapes::strokeCircle(center, 31, 4, ui::toColor(ui::SelectionRingColor));
	shapes::fillCircle(center, 28, WHITE);
	shapes::strokeCircle(center, 27, 2, ink);
	icons::drawFunction(functionType, Rectangle{ center.x - 15, center.y - 10, 30, 20 }, ink);
	// bias acima do neurônio, com a aparência do peso das conexões
	std::string prefix = "b ";
	std::string value = formatNumber(bias, 3);
	float size = ui::LabelFontSize;
	float prefixWidth = measureText(prefix, size).x;
	float width = prefixWidth + measureText(value, size).x + 16;
	biasBox = Rectangle{ center.x - width / 2, center.y - 56, width, 22 };
	if(biasChanged)
		drawPill(biasBox, ui::toColor(ui::ChangedColor), ui::toColor(ui::ChangedBorderColor));
	else if(biasSelected)
		drawPill(biasBox, ui::toColor(ui::AccentLightColor), ui::toColor(ui::AccentColor), 1.5f);
	else
		drawPill(biasBox, WHITE, ui::toColor(ui::PillBorderColor));
	float textY = biasBox.y + (biasBox.height - measureText(value, size).y) / 2;
	drawText(prefix, Vector2{ biasBox.x + 8, textY }, size, ui::toColor(ui::MutedColor));
	drawText(value, Vector2{ biasBox.x + 8 + prefixWidth, textY }, size,
		ui::toColor(ui::InkColor));
	if(!running)
		return;
	// valor calculado abaixo do neurônio
	std::string text = "y = " + formatNumber(getValue(), 2);
	Rectangle box = pillBox(Vector2{ center.x, center.y + 45 }, text, size, 20);
	Color dark = ui::toColor(ui::InkColor);
	drawPill(box, dark, dark);
	drawTextCentered(text, Vector2{ center.x, center.y + 45 }, size, WHITE);
}
