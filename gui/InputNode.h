#ifndef _INPUTNODE_H_
#define _INPUTNODE_H_
#include "Node.h"

class InputNode: public Node
{
public:
	InputNode(Texture2D img);
	virtual Type getType() { return TypeInput; }
	virtual bool IsIn(const Point& p);
	virtual void draw();
};

#endif /* _INPUTNODE_H_ */
