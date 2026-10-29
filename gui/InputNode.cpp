#include "InputNode.h"

InputNode::InputNode(Texture2D img): Node(img)
{
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
}
