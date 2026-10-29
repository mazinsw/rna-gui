#include "NeuronNode.h"
#include "Widgets.h"

NeuronNode::NeuronNode(Texture2D img, std::vector<Texture2D>& funcImgs): Node(img),
	funcImgs(funcImgs)
{
	setFunctionType(Linear);
	setBias(0);
	setBiasChanged(false);
	biasBox = Rectangle{ 0, 0, 0, 0 };
}

bool NeuronNode::IsIn(const Point& p)
{
	// dentro do círculo ou sobre o bias
	return sqrt(pow(getPosition().x - p.x, 2) + pow(getPosition().y - p.y, 2)) * 2 <=
		48 || CheckCollisionPointRec(Vector2{ (float)p.x, (float)p.y }, biasBox);
}

void NeuronNode::draw()
{
	if(isSelected())
		DrawCircle(getPosition().x, getPosition().y, 28, ORANGE);
	Node::draw();
	Texture2D img = funcImgs[functionType];
	DrawTexture(img, getPosition().x + 9, getPosition().y - (img.height / 2), WHITE);
	// bias acima do neurônio, com a aparência do peso das conexões
	std::string text = "b = " + formatNumber(bias, 3);
	Vector2 size = measureLabel(text, 13);
	biasBox = Rectangle{ getPosition().x - size.x / 2, (float)getPosition().y - 28 - size.y,
		size.x, size.y };
	Color fill = biasChanged ? Color{ 0xFF, 0xE5, 0x8F, 0xFF } : Color{ 0xE0, 0xF0, 0xE0, 0xFF };
	drawLabel(biasBox, text, 13, fill, Color{ 0, 127, 255, 255 }, Color{ 0, 0, 255, 255 });
}

void NeuronNode::drawValue()
{
	// abaixo do neurônio
	std::string text = "y = " + formatNumber(getValue(), 2);
	Vector2 size = measureLabel(text, 12);
	Rectangle box = { getPosition().x - size.x / 2, (float)getPosition().y + 34, size.x,
		size.y };
	drawLabel(box, text, 12, WHITE, ui::toColor(ui::BorderColor), Color{ 58, 58, 58, 255 });
}
