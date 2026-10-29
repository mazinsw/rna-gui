#include "NeuronNode.h"

NeuronNode::NeuronNode(Texture2D img, std::vector<Texture2D>& funcImgs): Node(img),
	funcImgs(funcImgs)
{
	setFunctionType(Linear);
}

bool NeuronNode::IsIn(const Point& p)
{
	// verifica se o ponto está dentro do círculo
	return sqrt(pow(getPosition().x - p.x, 2) + pow(getPosition().y - p.y, 2)) * 2 <=
		48;
}

void NeuronNode::draw()
{
	if(isSelected())
		DrawCircle(getPosition().x, getPosition().y, 28, ORANGE);
	Node::draw();
	Texture2D img = funcImgs[functionType];
	DrawTexture(img, getPosition().x + 9, getPosition().y - (img.height / 2), WHITE);
}
