#ifndef _NEURALNETWORK_H_
#define _NEURALNETWORK_H_
#include <raylib.h>
#include <random>
#include <stdlib.h>
#include <string>
#include <vector>
#include "Node.h"
#include "Edge.h"
#include "Execution.h"
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
	bool modified;
	// modo de execução: as entradas e as saídas alternam entre 0 e 1 ao clicar
	bool running;
	// entrada ou saída que alterna o valor se o botão for solto sem arrastar
	Node *nodeToggle;
	int pressX, pressY;
	Execution execution;
	std::mt19937 random;
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
	Node *valueAt(int x, int y);
	Edge* findEdge(Node* first, Node* second);
	Edge* edgeAt(int x, int y);
	bool canConnect(Node* first, Node* second);
	void clear();
	void draw();
	void toggle(Node *node);
	double randomWeight();
	NeuronNode *newNeuron();
	void runMouseDown(int x, int y);
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
		else if((key == KEY_DELETE || key == KEY_BACKSPACE) && !running)
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
				modified = true;
			}
		}
		else if(key == KEY_DOWN)
		{
			if(nodeSelected != NULL)
			{
				nodeSelected->setPosition(nodeSelected->getPosition().x,
					nodeSelected->getPosition().y + 1);
				modified = true;
			}
		}
		else if(key == KEY_LEFT)
		{
			if(nodeSelected != NULL)
			{
				nodeSelected->setPosition(nodeSelected->getPosition().x - 1,
					nodeSelected->getPosition().y);
				modified = true;
			}
		}
		else if(key == KEY_RIGHT)
		{
			if(nodeSelected != NULL)
			{
				nodeSelected->setPosition(nodeSelected->getPosition().x + 1,
					nodeSelected->getPosition().y);
				modified = true;
			}
		}
	}
	void mouseDown(int x, int y, int button)
	{
		if (button == MOUSE_BUTTON_LEFT)
		{
			if(running)
				return runMouseDown(x, y);
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
			// um clique sem arrastar alterna o valor da entrada ou da saída
			if(nodeToggle != NULL && abs(x - pressX) <= 3 && abs(y - pressY) <= 3)
				toggle(nodeToggle);
			nodeToggle = NULL;
			bMoving = false;
			bDown = false;
		}
	}

	/**
	 * Clique duplo no ponto: encerra a ação iniciada pelo primeiro clique e
	 * seleciona o neurônio ou a conexão sob o ponto. Retorna true se há algo
	 * para alterar.
	 */
	bool mouseDoubleClick(int x, int y);
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
			{
				nodeMoving->setPosition(this->x, this->y);
				modified = true;
			}
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
	/**
	 * Grava a rede, o caminho é UTF-8
	 */
	void saveToFile(const std::string& fileName);

	/**
	 * Substitui a rede pela do arquivo, o caminho é UTF-8. Se o arquivo for
	 * inválido lança exceção e a rede atual não é alterada.
	 */
	void loadFromFile(const std::string& fileName);

	/**
	 * Informa que um nó ou conexão foi alterado fora da rede (diálogos)
	 */
	void notifyDataChanged();

	/**
	 * Indica se há alterações desde a última leitura ou gravação
	 */
	bool isModified();
	Edge* getSelectedEdge();
	Node* getSelectedNode();

	/**
	 * Entra ou sai do modo de execução
	 */
	void setRunning(bool running);
	bool isRunning();

	/**
	 * Treina com os valores atuais das entradas e os alvos das saídas
	 */
	Execution::TrainResult train();

	/**
	 * Indica se está posicionando um nó novo
	 */
	bool isAdding();

	/**
	 * Indica se aguarda o clique no nó de destino de uma conexão
	 */
	bool isConnecting();
	bool hasCycle();
	double getError();
	bool isOnTarget();
};

#endif /* _NEURALNETWORK_H_ */
