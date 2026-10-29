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
 * coordenadas relativas ao seu canto superior esquerdo. A vista tem zoom e
 * deslocamento: os nós ficam em coordenadas da rede, convertidas na entrada.
 */
class NeuralNetwork
{
public:
	/**
	 * Pedido da área de edição para a janela: editar um peso ou um bias no
	 * lugar ou abrir o menu de um nó ou de uma conexão
	 */
	struct Request
	{
		enum Kind
		{
			None,
			EditWeight,
			EditBias,
			Menu
		};
		Kind kind;
		Node *node;
		Edge *edge;
	};
private:
	std::vector<Node *> neurons;
	std::vector<Edge *> edges;
	int x, y, downX, downY;
	bool bDown;
	bool bMoving;
	// o nó pressionado já saiu do lugar, um clique com tremor não o move
	bool dragging;
	// alinhamento do nó arrastado ou posicionado em cada eixo
	int snapX, snapY;
	// vista: posição na área de edição = posição na rede * zoom + pan
	float zoom;
	Vector2 pan;
	// arrastando a vista, com a última posição do mouse na área de edição
	bool panning;
	int panX, panY;
	int actionState;
	Node *nodeAdd;
	Edge *edgeAdd;
	Node *nodeMoving;
	Node *nodeSelected;
	Edge * edgeSelected;
	// neurônio com o rótulo do bias selecionado
	NeuronNode *biasSelected;
	bool modified;
	// modo de execução: as entradas e as saídas alternam entre 0 e 1 ao clicar
	bool running;
	// entrada ou saída que alterna o valor se o botão for solto sem arrastar
	Node *nodeToggle;
	int pressX, pressY;
	Request request;
	Execution execution;
	std::mt19937 random;
private:
	enum ActionState
	{
		ActionNone = 0,
		ActionAdd,
		ActionConnect
	};
	enum Snap
	{
		SnapNone = 0,
		// ponto da grade
		SnapGrid,
		// camada (coluna) ou linha de outro nó
		SnapLayer
	};
private:
	void selectNode(Node *node);
	void selectEdge(Edge *edge);
	void selectBias(NeuronNode *neuron);
	void addNeuron(int x, int y);
	void startAdd(Node *node);
	void addEdge(Node *node);
	void updateAddNeuron(int x, int y);
	void updateAddEdge(int x, int y);
	Node *neuronAt(int x, int y);
	Node *valueAt(int x, int y);
	NeuronNode *biasAt(int x, int y);
	Edge* findEdge(Node* first, Node* second);
	Edge* edgeAt(int x, int y);
	bool canConnect(Node* first, Node* second);
	void clear();
	void draw();

	/**
	 * Linhas de alinhamento do nó arrastado ou posicionado, na tela, nos
	 * limites da área de edição
	 */
	void drawGuides(Rectangle bounds);

	/**
	 * Posição na rede de um ponto da área de edição
	 */
	Point toWorld(int x, int y);
	void startPanning(int x, int y);

	/**
	 * Posição do nó alinhada à camada ou à linha de um nó próximo e, sem
	 * nenhum, à grade. Com Alt (Option no Mac) fica livre.
	 */
	Point snapPosition(Node *node, int x, int y);
	void toggle(Node *node);
	double randomWeight();
	NeuronNode *newNeuron();
	void pressNode(Node *node, int x, int y);
	void updateInfluence();

	/**
	 * Encerra a conexão em andamento e o arraste
	 */
	void cancelAction();
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
				removeNode(nodeSelected);
			else if(edgeSelected != NULL)
				removeEdge(edgeSelected);
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

	/**
	 * Botão esquerdo pressionado: no nó seleciona, arrasta e conecta (na
	 * execução alterna a entrada ou o alvo da saída); no peso ou no bias o
	 * primeiro clique seleciona e o segundo pede a edição
	 */
	void mouseDown(int x, int y);
	void mouseUp(int x, int y)
	{
		// um clique sem arrastar alterna o valor da entrada ou da saída
		if(nodeToggle != NULL && abs(x - pressX) <= 3 && abs(y - pressY) <= 3)
			toggle(nodeToggle);
		nodeToggle = NULL;
		bMoving = false;
		dragging = false;
		panning = false;
		bDown = false;
	}

	/**
	 * Clique duplo: pede o menu do neurônio ou da entrada e a edição do peso
	 * ou do bias
	 */
	void mouseDoubleClick(int x, int y);

	/**
	 * Botão direito: seleciona e pede o menu do nó ou da conexão
	 */
	void mouseRightDown(int x, int y);
	void mouseMove(int x, int y)
	{
		if(panning)
		{
			pan.x += x - panX;
			pan.y += y - panY;
			panX = x;
			panY = y;
			return;
		}
		// o tremor do clique é medido na tela, o arraste do nó na rede
		Point p = toWorld(x, y);
		if(actionState == ActionAdd)
		{
			updateAddNeuron(p.x, p.y);
		}
		else if (bDown)
		{
			this->x += p.x - downX;
			this->y += p.y - downY;
			downX = p.x;
			downY = p.y;
			if(bMoving && !dragging && (abs(x - pressX) > 3 || abs(y - pressY) > 3))
			{
				dragging = true;
				actionState = ActionNone;
			}
			if(dragging)
			{
				Point p = snapPosition(nodeMoving, this->x, this->y);
				if(p.x != nodeMoving->getPosition().x || p.y != nodeMoving->getPosition().y)
				{
					nodeMoving->setPosition(p);
					modified = true;
				}
			}
		}
		else if(actionState == ActionConnect)
		{
			updateAddEdge(p.x, p.y);
		}
	}
	/**
	 * Desenha a área de edição nos limites informados
	 */
	void paint(Rectangle bounds);

	/**
	 * Indica se o botão do mouse foi pressionado na área de edição e ainda
	 * não foi solto, ou se a vista está sendo arrastada
	 */
	inline bool isMouseDown()
	{
		return bDown || panning;
	}

	/**
	 * Zoom da roda do mouse, o ponto da rede sob o mouse fica parado
	 */
	void zoomAt(int x, int y, float wheel);

	/**
	 * Botão do meio pressionado: arrasta a vista mesmo sobre um nó
	 */
	void startPan(int x, int y);
	void endPan();

	/**
	 * Posição na área de edição de um ponto da rede
	 */
	Vector2 toScreen(Vector2 point);
	Rectangle toScreen(Rectangle rect);

	/**
	 * Centraliza a rede na área, com o zoom que a mostra inteira, sem passar
	 * de 100%. A área é a dos centros dos nós, na área de edição.
	 */
	void fitView(Rectangle area);
public:
	NeuralNetwork();
	~NeuralNetwork();
	void addNeuron();
	void addInput();
	void addOutput();

	/**
	 * Exclui o nó e as conexões dele
	 */
	void removeNode(Node *node);
	void removeEdge(Edge *edge);
	void setFunction(NeuronNode *neuron, int function);

	/**
	 * Desloca a rede para a direita até nenhum nó ficar antes de left, a
	 * posição não conta como alteração
	 */
	void keepRightOf(int left);

	/**
	 * Reorganiza a rede em camadas: as entradas na primeira, cada neurônio
	 * depois dos nós que o alimentam e as saídas na última, centralizadas na
	 * área informada para os centros dos nós e alinhadas à grade. Sem espaço
	 * para a distância mínima entre os nós a rede passa da área, e fitView a
	 * mostra inteira. Retorna o número de camadas.
	 */
	int arrange(Rectangle area);
	void setInputBias(InputNode *input, bool bias);
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
	 * Informa que um nó ou conexão foi alterado fora da rede (edição no lugar)
	 */
	void notifyDataChanged();

	/**
	 * Indica se há alterações desde a última leitura ou gravação
	 */
	bool isModified();
	Edge* getSelectedEdge();
	Node* getSelectedNode();
	NeuronNode* getSelectedBias();

	/**
	 * Retorna o pedido feito pelos eventos de mouse e o descarta
	 */
	Request takeRequest();

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
	 * Indica se um nó está sendo arrastado
	 */
	bool isMoving();

	/**
	 * Indica se aguarda o clique no nó de destino de uma conexão
	 */
	bool isConnecting();
	bool hasCycle();
	double getError();
	bool isOnTarget();
};

#endif /* _NEURALNETWORK_H_ */
