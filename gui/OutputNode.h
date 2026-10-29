#ifndef _OUTPUTNODE_H_
#define _OUTPUTNODE_H_
#include "Node.h"

class OutputNode: public Node
{
private:

protected:

public:
	OutputNode(Texture2D img);
	virtual Type getType() { return TypeOutput; }
	virtual bool IsIn(const Point& p);
	virtual void draw();
};

#endif /* _OUTPUTNODE_H_ */
