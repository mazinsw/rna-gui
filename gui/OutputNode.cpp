#include "OutputNode.h"

OutputNode::OutputNode(Texture2D img): Node(img)
{
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
