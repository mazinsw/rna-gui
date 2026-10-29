#include "Node.h"

Node::Node(Texture2D img)
{
	this->img = img;
	setSelected(false);
}

void Node::draw()
{
	DrawTexture(img, p.x - img.width / 2, p.y - img.height / 2, WHITE);
}
