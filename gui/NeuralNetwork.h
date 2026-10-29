#ifndef _NEURALNETWORK_H_
#define _NEURALNETWORK_H_
#include <raylib.h>
#include <string>
#include <vector>
#include "Node.h"
#include "Edge.h"
#include "InputNode.h"
#include "NeuronNode.h"
#include "OutputNode.h"

/**
 * Área de edição da rede, recebe os eventos de mouse e teclado com
 * coordenadas relativas ao seu canto superior esquerdo
 */
class NeuralNetwork
{
private:
	Texture2D neuronImg;
	Texture2D inputImg;
	Texture2D outputImg;
	std::vector<Texture2D> funcImgs;
	std::vector<Node *> neurons;
	std::vector<Edge *> edges;
	int x, y, downX, downY;
	bool bDown;
	bool bMoving;
	int actionState;
	Node *nodeAdd;
	Edge *edgeAdd;
	Node *nodeMoving;
	Node *nodeSelected;
	Edge * edgeSelected;
private:
	enum ActionState
	{
		ActionNone = 0,
		ActionAdd,
		ActionConnect
	};
private:
	void selectNode(Node *node);
	void selectEdge(Edge *edge);
	void deleteNode(Node *node);
	void deleteEdge(Edge *edge);
	void addNeuron(int x, int y);
	void addEdge(Node *node);
	void updateAddNeuron(int x, int y);
	void updateAddEdge(int x, int y);
	Node *neuronAt(int x, int y);
	Edge* findEdge(Node* first, Node* second);
	Edge* edgeAt(int x, int y);
	void draw();
public:
	void keyDown(int key)
	{
		if(key == KEY_ESCAPE)
		{
			if(actionState == ActionConnect)
			{
				actionState = ActionNone;
			}
			else if(actionState == ActionAdd)
			{
				actionState = ActionNone;
			}
		}
		// no teclado do Mac a tecla delete é o backspace
		else if(key == KEY_DELETE || key == KEY_BACKSPACE)
		{
			if(nodeSelected != NULL)
			{
				Node * saveNodeSelected = nodeSelected;
				selectNode(NULL);
				deleteNode(saveNodeSelected);
			}
			else if(edgeSelected != NULL)
			{
				Edge * saveEdgeSelected = edgeSelected;
				selectEdge(NULL);
				deleteEdge(saveEdgeSelected);
			}
		}
		else if(key == KEY_UP)
		{
			if(nodeSelected != NULL)
			{
				nodeSelected->setPosition(nodeSelected->getPosition().x,
					nodeSelected->getPosition().y - 1);
			}
		}
		else if(key == KEY_DOWN)
		{
			if(nodeSelected != NULL)
			{
				nodeSelected->setPosition(nodeSelected->getPosition().x,
					nodeSelected->getPosition().y + 1);
			}
		}
		else if(key == KEY_LEFT)
		{
			if(nodeSelected != NULL)
			{
				nodeSelected->setPosition(nodeSelected->getPosition().x - 1,
					nodeSelected->getPosition().y);
			}
		}
		else if(key == KEY_RIGHT)
		{
			if(nodeSelected != NULL)
			{
				nodeSelected->setPosition(nodeSelected->getPosition().x + 1,
					nodeSelected->getPosition().y);
			}
		}
	}
	void mouseDown(int x, int y, int button)
	{
		if (button == MOUSE_BUTTON_LEFT)
		{
			if(actionState == ActionAdd)
			{
				addNeuron(x, y);
				actionState = ActionNone;
				return;
			}
			Node *node = neuronAt(x, y);
			if(node != NULL)
			{
				selectEdge(NULL);
				selectNode(node);
				bMoving = true;
				nodeMoving = node;
				if(actionState == ActionConnect)
				{
					actionState = ActionNone;
					addEdge(node);
				}
				else
				{
					actionState = ActionConnect;
					edgeAdd->setFirst(node);
					nodeAdd->setPosition(x, y);
				}
				this->x = node->getPosition().x;
				this->y = node->getPosition().y;
			}
			else
			{
				selectNode(node);
				actionState = ActionNone;
				Edge * edge = edgeAt(x, y);
				selectEdge(edge);
			}
			downX = x;
			downY = y;
			bDown = true;
		}
	}
	void mouseUp(int x, int y, int button)
	{
		if (button == MOUSE_BUTTON_LEFT)
		{
			bMoving = false;
			bDown = false;
		}
	}
	void mouseMove(int x, int y)
	{
		if(actionState == ActionAdd)
		{
			updateAddNeuron(x, y);
		}
		else if (bDown)
		{
			this->x += x - downX;
			this->y += y - downY;
			downX = x;
			downY = y;
			actionState = ActionNone;
			if(bMoving)
				nodeMoving->setPosition(this->x, this->y);
		}
		else if(actionState == ActionConnect)
		{
			updateAddEdge(x, y);
		}
	}
	/**
	 * Desenha a área de edição nos limites informados
	 */
	void paint(Rectangle bounds);

	/**
	 * Indica se o botão do mouse foi pressionado na área de edição e ainda
	 * não foi solto
	 */
	inline bool isMouseDown()
	{
		return bDown;
	}
public:
	NeuralNetwork();
	~NeuralNetwork();
	void addNeuron();
	void addInput();
	void addOutput();
	void saveToFile(const std::string& fileName);
	void loadFromFile(const std::string& fileName);
	Edge* getSelectedEdge();
	Node* getSelectedNode();
};

#endif /* _NEURALNETWORK_H_ */
