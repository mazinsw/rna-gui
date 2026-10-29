#ifndef _NEURONNODE_H_
#define _NEURONNODE_H_
#include <vector>
#include "Node.h"

class NeuronNode: public Node
{
private:
	int functionType;
	std::vector<Texture2D>& funcImgs;
protected:
public:
	enum FunctionType
	{
		Linear = 0,
		Degrau,
		Bipolar,
		Logistic,
		Hiperbolic,
		FunctionCount
	};
public:
	NeuronNode(Texture2D img, std::vector<Texture2D>& funcImgs);
	virtual Type getType() { return TypeNeuron; }
	virtual bool IsIn(const Point& p);
	virtual void draw();
	virtual void drawValue();

	inline int getFunctionType()
	{
		return functionType;
	}

	inline void setFunctionType(int functionType)
	{
		this->functionType = functionType;
	}
};

#endif /* _NEURONNODE_H_ */
