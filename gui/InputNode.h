#ifndef _INPUTNODE_H_
#define _INPUTNODE_H_
#include "Node.h"

class InputNode: public Node
{
private:
	Rectangle valueBox;
public:
	InputNode(Texture2D img);
	virtual Type getType() { return TypeInput; }
	virtual bool IsIn(const Point& p);
	virtual void draw();
	virtual void drawValue();
	virtual bool IsInValue(const Point& p);

	/**
	 * Alterna o valor da entrada entre 0 e 1
	 */
	inline void toggleValue()
	{
		setValue(getValue() != 0 ? 0 : 1);
	}
};

#endif /* _INPUTNODE_H_ */
