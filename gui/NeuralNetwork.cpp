#include "NeuralNetwork.h"
#include "Platform.h"
#include "Shapes.h"
#include "Widgets.h"
#include <cmath>
#include <ctype.h>
#include <algorithm>
#include <fstream>
#include <functional>
#include <map>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#define RN_SIGN "1.1"
#define RN_SIGN_BIAS "1.2"
// pontos da grade a cada GRID_SIZE pixels, o primeiro em GRID_OFFSET
#define GRID_SIZE 20
#define GRID_OFFSET 10
// distância em que o nó arrastado entra na camada ou na linha de outro nó
#define SNAP_DISTANCE 8
// maior distância entre camadas e entre os nós de uma camada ao reorganizar
#define ARRANGE_LAYER_SPACING 200
#define ARRANGE_NODE_SPACING 160

/*
 * Formato do arquivo, little-endian com tamanhos fixos para ser o mesmo em
 * qualquer plataforma. A versão 1.1 é a gravada pela versão Windows 32 bits:
 * sign[3] neuronCount:u32 edgeCount:u32
 * neuronCount x (type:i32 function:i32 x:i32 y:i32)
 * edgeCount x (first:i32 second:i32 weight:f64)
 * A versão 1.2 acrescenta bias:f64 a cada nó e usa function da entrada para
 * marcar a entrada de bias (1). É gravada só quando a rede usa algum bias, as
 * demais continuam em 1.1.
 */

static void writeUInt32(std::ofstream& file, uint32_t value)
{
	unsigned char bytes[4];
	for(int i = 0; i < 4; i++)
		bytes[i] = (unsigned char)(value >> (i * 8));
	file.write((const char*)bytes, sizeof(bytes));
}

static void writeInt32(std::ofstream& file, int32_t value)
{
	writeUInt32(file, (uint32_t)value);
}

static void writeDouble(std::ofstream& file, double value)
{
	uint64_t bits;
	unsigned char bytes[8];
	memcpy(&bits, &value, sizeof(bits));
	for(int i = 0; i < 8; i++)
		bytes[i] = (unsigned char)(bits >> (i * 8));
	file.write((const char*)bytes, sizeof(bytes));
}

static void readBytes(std::ifstream& file, unsigned char* bytes, size_t count)
{
	file.read((char*)bytes, count);
	if(!file)
		throw std::runtime_error("Unexpected end of file.");
}

static uint32_t readUInt32(std::ifstream& file)
{
	unsigned char bytes[4];
	uint32_t value = 0;
	readBytes(file, bytes, sizeof(bytes));
	for(int i = 0; i < 4; i++)
		value |= (uint32_t)bytes[i] << (i * 8);
	return value;
}

static int32_t readInt32(std::ifstream& file)
{
	return (int32_t)readUInt32(file);
}

static double readDouble(std::ifstream& file)
{
	unsigned char bytes[8];
	uint64_t bits = 0;
	double value;
	readBytes(file, bytes, sizeof(bytes));
	for(int i = 0; i < 8; i++)
		bits |= (uint64_t)bytes[i] << (i * 8);
	memcpy(&value, &bits, sizeof(value));
	return value;
}

NeuralNetwork::NeuralNetwork(): execution(neurons, edges), random(std::random_device()())
{
	bDown = false;
	bMoving = false;
	dragging = false;
	snapX = snapY = SnapNone;
	x = y = downX = downY = 0;
	nodeMoving = NULL;
	nodeSelected = NULL;
	edgeSelected = NULL;
	biasSelected = NULL;
	nodeAdd = newNeuron();
	edgeAdd = new Edge(NULL, nodeAdd, 0);
	actionState = ActionNone;
	modified = false;
	running = false;
	nodeToggle = NULL;
	pressX = pressY = 0;
	request = Request{ Request::None, NULL, NULL };
}

NeuralNetwork::~NeuralNetwork()
{
	clear();
	delete edgeAdd;
	delete nodeAdd;
}

NeuronNode *NeuralNetwork::newNeuron()
{
	NeuronNode *neuron = new NeuronNode();
	neuron->setBias(randomWeight());
	return neuron;
}

void NeuralNetwork::startAdd(Node *node)
{
	actionState = ActionAdd;
	delete nodeAdd;
	nodeAdd = node;
	edgeAdd->setSecond(nodeAdd);
	// sem linhas até o mouse posicionar o nó
	snapX = snapY = SnapNone;
}

void NeuralNetwork::addNeuron()
{
	startAdd(newNeuron());
}

void NeuralNetwork::addInput()
{
	startAdd(new InputNode());
}

void NeuralNetwork::addOutput()
{
	startAdd(new OutputNode());
}

void NeuralNetwork::addNeuron(int x, int y)
{
	Node *node = nodeAdd;
	node->setPosition(snapPosition(node, x, y));
	neurons.push_back(node);
	nodeAdd = newNeuron();
	edgeAdd->setSecond(nodeAdd);
	modified = true;
}

void NeuralNetwork::paint(Rectangle bounds)
{
	DrawRectangleRec(bounds, ui::toColor(ui::CanvasColor));
	BeginScissorMode((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height);
	// grade de pontos, onde os nós se alinham
	Color grid = ui::toColor(ui::GridColor);
	for(float gy = bounds.y + GRID_OFFSET; gy < bounds.y + bounds.height; gy += GRID_SIZE)
	{
		for(float gx = bounds.x + GRID_OFFSET; gx < bounds.x + bounds.width; gx += GRID_SIZE)
			DrawRectangleRec(Rectangle{ gx - 0.75f, gy - 0.75f, 1.5f, 1.5f }, grid);
	}
	// desenha com a origem no canto da área de edição
	Camera2D camera = { { bounds.x, bounds.y }, { 0, 0 }, 0, 1 };
	BeginMode2D(camera);
	drawGuides(bounds.width, bounds.height);
	draw();
	EndMode2D();
	EndScissorMode();
}

/**
 * Linha tracejada de 1 pixel
 */
static void dashedLine(Vector2 a, Vector2 b, Color color)
{
	const float dash = 4, gap = 4;
	float length = hypotf(b.x - a.x, b.y - a.y);
	if(length <= 0)
		return;
	Vector2 direction = { (b.x - a.x) / length, (b.y - a.y) / length };
	for(float d = 0; d < length; d += dash + gap)
	{
		float end = fminf(d + dash, length);
		shapes::line(Vector2{ a.x + direction.x * d, a.y + direction.y * d },
			Vector2{ a.x + direction.x * end, a.y + direction.y * end }, 1, color);
	}
}

void NeuralNetwork::drawGuides(float width, float height)
{
	Node *node = NULL;
	if(dragging)
		node = nodeMoving;
	else if(actionState == ActionAdd)
		node = nodeAdd;
	if(node == NULL)
		return;
	// por baixo das conexões e dos nós: azul na camada ou na linha de outro
	// nó, tracejada no ponto da grade
	Vector2 center = node->getCenter();
	Color layer = ui::toColor(ui::AccentColor);
	Color grid = ui::toColor(ui::FaintColor);
	Vector2 top = { center.x, 0 }, bottom = { center.x, height };
	Vector2 left = { 0, center.y }, right = { width, center.y };
	if(snapX == SnapLayer)
		shapes::line(top, bottom, 1, layer);
	else if(snapX == SnapGrid)
		dashedLine(top, bottom, grid);
	if(snapY == SnapLayer)
		shapes::line(left, right, 1, layer);
	else if(snapY == SnapGrid)
		dashedLine(left, right, grid);
}

static int snapToGrid(int value)
{
	return (int)floorf((value - GRID_OFFSET) / (float)GRID_SIZE + 0.5f) * GRID_SIZE + GRID_OFFSET;
}

Point NeuralNetwork::snapPosition(Node *node, int x, int y)
{
	snapX = snapY = SnapNone;
	if(IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))
		return Point(x, y);
	Point p(snapToGrid(x), snapToGrid(y));
	snapX = snapY = SnapGrid;
	// a camada mais próxima tem prioridade sobre a grade, para alinhar com nós
	// que estão fora dela
	int distanceX = SNAP_DISTANCE + 1, distanceY = SNAP_DISTANCE + 1;
	std::vector<Node *>::iterator it;
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		if(*it == node)
			continue;
		const Point& other = (*it)->getPosition();
		if(abs(other.x - x) < distanceX)
		{
			distanceX = abs(other.x - x);
			p.x = other.x;
			snapX = SnapLayer;
		}
		if(abs(other.y - y) < distanceY)
		{
			distanceY = abs(other.y - y);
			p.y = other.y;
			snapY = SnapLayer;
		}
	}
	return p;
}

void NeuralNetwork::updateInfluence()
{
	// na edição pelo peso, na execução pelo sinal que passa na conexão
	std::vector<double> influences;
	double highest = 0;
	std::vector<Edge *>::iterator ite;
	for(ite = edges.begin(); ite != edges.end(); ite++)
	{
		Edge *edge = *ite;
		double influence = fabs(edge->getWeight());
		if(running)
			influence *= fabs(edge->getFirst()->getValue());
		if(!std::isfinite(influence))
			influence = 0;
		influences.push_back(influence);
		if(influence > highest)
			highest = influence;
	}
	for(size_t i = 0; i < edges.size(); i++)
		edges[i]->setInfluence(highest > 0 ? (float)(influences[i] / highest) : 0);
}

void NeuralNetwork::draw()
{
	updateInfluence();
	std::vector<Edge *>::iterator ite;
	for(ite = edges.begin(); ite != edges.end(); ite++)
		(*ite)->drawLine();
	if(!bMoving && actionState == ActionConnect)
		shapes::line(edgeAdd->getFirst()->getCenter(), nodeAdd->getCenter(), 2,
			ui::toColor(ui::FaintColor));
	for(ite = edges.begin(); ite != edges.end(); ite++)
		(*ite)->drawLabel();
	std::vector<Node *>::iterator it;
	for(it = neurons.begin(); it != neurons.end(); it++)
		(*it)->draw(running);
	if(actionState == ActionAdd)
		nodeAdd->draw(false);
}

Node *NeuralNetwork::neuronAt(int x, int y)
{
	std::vector<Node *>::iterator it;
	Point p(x, y);
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		Node *node = *it;
		if(node->IsIn(p))
			return node;
	}
	return NULL;
}

Node *NeuralNetwork::valueAt(int x, int y)
{
	std::vector<Node *>::iterator it;
	Point p(x, y);
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		Node *node = *it;
		if(node->IsInValue(p))
			return node;
	}
	return NULL;
}

NeuronNode *NeuralNetwork::biasAt(int x, int y)
{
	std::vector<Node *>::iterator it;
	Point p(x, y);
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		Node *node = *it;
		if(node->is(Node::TypeNeuron) && ((NeuronNode*)node)->IsInBias(p))
			return (NeuronNode*)node;
	}
	return NULL;
}

void NeuralNetwork::updateAddNeuron(int x, int y)
{
	nodeAdd->setPosition(snapPosition(nodeAdd, x, y));
}

void NeuralNetwork::updateAddEdge(int x, int y)
{
	nodeAdd->setPosition(x, y);
}

Edge *NeuralNetwork::findEdge(Node *first, Node *second)
{
	std::vector<Edge *>::iterator ite;
	for(ite = edges.begin(); ite != edges.end(); ite++)
	{
		Edge *edge = *ite;
		if(edge->getFirst() == first && edge->getSecond() == second)
			return edge;
	}
	return NULL;
}

Edge *NeuralNetwork::edgeAt(int x, int y)
{
	std::vector<Edge *>::iterator ite;
	Point p(x, y);
	// os rótulos ficam por cima das linhas
	for(ite = edges.begin(); ite != edges.end(); ite++)
	{
		if((*ite)->IsInLabel(p))
			return *ite;
	}
	for(ite = edges.begin(); ite != edges.end(); ite++)
	{
		if((*ite)->IsIn(p))
			return *ite;
	}
	return NULL;
}

bool NeuralNetwork::canConnect(Node *first, Node *second)
{
	// o segundo clique de um clique duplo não liga o nó a ele mesmo
	if(first == second)
		return false;
	if(findEdge(first, second) != NULL)
		return false;
	if(first->is(Node::TypeInput) && second->is(Node::TypeInput))
		return false;
	if(first->is(Node::TypeOutput) && second->is(Node::TypeOutput))
		return false;
	if(first->is(Node::TypeOutput) && second->is(Node::TypeInput))
		return false;
	if(first->is(Node::TypeInput) && second->is(Node::TypeOutput))
		return false;
	return true;
}

void NeuralNetwork::addEdge(Node *node)
{
	Node *first = edgeAdd->getFirst();
	if(!canConnect(first, node))
		return;
	// a conexão com a saída repassa o valor do neurônio, as demais começam
	// com peso aleatório
	double weight = node->is(Node::TypeOutput) ? 1 : randomWeight();
	Edge *edge = new Edge(first, node, weight);
	edges.push_back(edge);
	modified = true;
}

double NeuralNetwork::randomWeight()
{
	// entre -1 e 1 com 2 casas
	std::uniform_int_distribution<int> distribution(-100, 100);
	return distribution(random) / 100.0;
}

void NeuralNetwork::selectNode(Node *node)
{
	if(nodeSelected == node)
		return;
	if(nodeSelected != NULL)
		nodeSelected->setSelected(false);
	nodeSelected = node;
	if(nodeSelected == NULL)
		return;
	nodeSelected->setSelected(true);
}

void NeuralNetwork::selectEdge(Edge* edge)
{
	if(edgeSelected == edge)
		return;
	if(edgeSelected != NULL)
		edgeSelected->setSelected(false);
	edgeSelected = edge;
	if(edgeSelected == NULL)
		return;
	edgeSelected->setSelected(true);
}

void NeuralNetwork::selectBias(NeuronNode* neuron)
{
	if(biasSelected == neuron)
		return;
	if(biasSelected != NULL)
		biasSelected->setBiasSelected(false);
	biasSelected = neuron;
	if(biasSelected == NULL)
		return;
	biasSelected->setBiasSelected(true);
}

void NeuralNetwork::removeNode(Node *node)
{
	if(nodeSelected == node)
		selectNode(NULL);
	if(biasSelected == node)
		selectBias(NULL);
	if(nodeMoving == node || nodeToggle == node)
	{
		nodeMoving = nodeToggle = NULL;
		bMoving = dragging = bDown = false;
	}
	if(edgeAdd->getFirst() == node)
		actionState = ActionNone;
	std::vector<Edge *>::iterator ite;
	ite = edges.begin();
	while(ite != edges.end())
	{
		Edge *edge = *ite;
		if(edge->getFirst() != node && edge->getSecond() != node)
		{
			ite++;
			continue;
		}
		if(edgeSelected == edge)
			selectEdge(NULL);
		delete *ite;
		ite = edges.erase(ite);
	}
	std::vector<Node *>::iterator itn;
	for(itn = neurons.begin(); itn != neurons.end(); itn++)
	{
		Node *nodeIte = *itn;
		if(nodeIte != node)
			continue;
		delete nodeIte;
		neurons.erase(itn);
		break;
	}
	actionState = ActionNone;
	modified = true;
}

void NeuralNetwork::removeEdge(Edge* edge)
{
	if(edgeSelected == edge)
		selectEdge(NULL);
	std::vector<Edge *>::iterator ite;
	for(ite = edges.begin(); ite != edges.end(); ite++)
	{
		Edge *edgeIte = *ite;
		if(edgeIte != edge)
			continue;
		delete edgeIte;
		edges.erase(ite);
		break;
	}
	actionState = ActionNone;
	modified = true;
}

void NeuralNetwork::setFunction(NeuronNode *neuron, int function)
{
	neuron->setFunctionType(function);
	notifyDataChanged();
}

void NeuralNetwork::keepRightOf(int left)
{
	if(neurons.empty())
		return;
	int lowest = neurons[0]->getPosition().x;
	std::vector<Node *>::iterator it;
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		if((*it)->getPosition().x < lowest)
			lowest = (*it)->getPosition().x;
	}
	if(lowest >= left)
		return;
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		const Point& p = (*it)->getPosition();
		(*it)->setPosition(p.x + left - lowest, p.y);
	}
}

/**
 * Maior espaçamento até max, múltiplo da grade, para count nós caberem em size
 */
static int arrangeSpacing(int count, float size, int max)
{
	if(count < 2)
		return 0;
	int spacing = (int)(size / (count - 1)) / GRID_SIZE * GRID_SIZE;
	if(spacing > max)
		spacing = max;
	return spacing < GRID_SIZE ? GRID_SIZE : spacing;
}

int NeuralNetwork::arrange(Rectangle area)
{
	cancelAction();
	if(neurons.empty())
		return 0;
	std::map<Node *, std::vector<Node *> > sources;
	std::vector<Edge *>::iterator ite;
	for(ite = edges.begin(); ite != edges.end(); ite++)
		sources[(*ite)->getSecond()].push_back((*ite)->getFirst());
	// camada do neurônio: uma depois do nó mais distante que o alimenta
	std::map<Node *, int> layers;
	std::function<int(Node *)> layerOf = [&](Node *node) -> int
	{
		std::map<Node *, int>::iterator found = layers.find(node);
		if(found != layers.end())
			return found->second;
		if(!node->is(Node::TypeNeuron))
			return 0;
		// num ciclo a volta ao neurônio encontra este valor e para
		layers[node] = 1;
		int layer = 1;
		std::vector<Node *>::iterator its;
		for(its = sources[node].begin(); its != sources[node].end(); its++)
			layer = std::max(layer, layerOf(*its) + 1);
		return layers[node] = layer;
	};
	int outputLayer = 1;
	std::vector<Node *>::iterator it;
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		if((*it)->is(Node::TypeNeuron))
			outputLayer = std::max(outputLayer, layerOf(*it) + 1);
	}
	// colunas na ordem atual de cima para baixo, sem as camadas vazias
	std::vector<std::vector<Node *> > columns(outputLayer + 1);
	std::vector<Node *> sorted = neurons;
	std::stable_sort(sorted.begin(), sorted.end(), [](Node *a, Node *b)
	{
		return a->getPosition().y < b->getPosition().y;
	});
	for(it = sorted.begin(); it != sorted.end(); it++)
	{
		Node *node = *it;
		int layer = 0;
		if(node->is(Node::TypeNeuron))
			layer = layers[node];
		else if(node->is(Node::TypeOutput))
			layer = outputLayer;
		columns[layer].push_back(node);
	}
	columns.erase(std::remove_if(columns.begin(), columns.end(),
		[](const std::vector<Node *>& column) { return column.empty(); }), columns.end());
	int spacingX = arrangeSpacing((int)columns.size(), area.width, ARRANGE_LAYER_SPACING);
	int left = snapToGrid((int)(area.x + (area.width - spacingX * ((int)columns.size() - 1)) / 2));
	std::map<Node *, Point> positions;
	for(size_t c = 0; c < columns.size(); c++)
	{
		std::vector<Node *>& column = columns[c];
		// cada nó perto da altura média dos que o alimentam, para cruzar menos
		std::map<Node *, float> heights;
		for(it = column.begin(); it != column.end(); it++)
		{
			float sum = 0;
			int count = 0;
			std::vector<Node *>::iterator its;
			for(its = sources[*it].begin(); its != sources[*it].end(); its++)
			{
				std::map<Node *, Point>::iterator placed = positions.find(*its);
				if(placed == positions.end())
					continue;
				sum += placed->second.y;
				count++;
			}
			heights[*it] = count > 0 ? sum / count : (*it)->getPosition().y;
		}
		std::stable_sort(column.begin(), column.end(), [&](Node *a, Node *b)
		{
			return heights[a] < heights[b];
		});
		int spacingY = arrangeSpacing((int)column.size(), area.height, ARRANGE_NODE_SPACING);
		int top = snapToGrid((int)(area.y + (area.height - spacingY * ((int)column.size() - 1)) / 2));
		for(size_t n = 0; n < column.size(); n++)
			positions[column[n]] = Point(left + spacingX * (int)c, top + spacingY * (int)n);
	}
	std::map<Node *, Point>::iterator itp;
	for(itp = positions.begin(); itp != positions.end(); itp++)
	{
		const Point& old = itp->first->getPosition();
		if(old.x == itp->second.x && old.y == itp->second.y)
			continue;
		itp->first->setPosition(itp->second);
		modified = true;
	}
	return (int)columns.size();
}

void NeuralNetwork::setInputBias(InputNode *input, bool bias)
{
	input->setBias(bias);
	notifyDataChanged();
}

void NeuralNetwork::clear()
{
	selectNode(NULL);
	selectEdge(NULL);
	selectBias(NULL);
	std::vector<Edge *>::iterator ite;
	for(ite = edges.begin(); ite != edges.end(); ite++)
		delete *ite;
	edges.clear();
	std::vector<Node *>::iterator it;
	for(it = neurons.begin(); it != neurons.end(); it++)
		delete *it;
	neurons.clear();
	edgeAdd->setFirst(NULL);
	nodeMoving = NULL;
	nodeToggle = NULL;
	bDown = false;
	bMoving = false;
	dragging = false;
	actionState = ActionNone;
	request = Request{ Request::None, NULL, NULL };
}

void NeuralNetwork::notifyDataChanged()
{
	modified = true;
	if(!running)
		return;
	execution.clearChanged();
	execution.execute();
}

void NeuralNetwork::cancelAction()
{
	actionState = ActionNone;
	nodeToggle = NULL;
	bMoving = false;
	dragging = false;
	bDown = false;
}

void NeuralNetwork::pressNode(Node *node, int x, int y)
{
	selectEdge(NULL);
	selectBias(NULL);
	if(running)
	{
		// entradas e saídas alternam o valor, neurônios são selecionados
		if(node->is(Node::TypeNeuron))
			selectNode(node);
		else
			nodeToggle = node;
	}
	else
	{
		selectNode(node);
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
	}
	bMoving = true;
	nodeMoving = node;
	this->x = node->getPosition().x;
	this->y = node->getPosition().y;
}

void NeuralNetwork::mouseDown(int x, int y)
{
	downX = pressX = x;
	downY = pressY = y;
	nodeToggle = NULL;
	dragging = false;
	if(actionState == ActionAdd)
	{
		addNeuron(x, y);
		actionState = ActionNone;
		return;
	}
	Node *node = neuronAt(x, y);
	if(node != NULL)
	{
		pressNode(node, x, y);
		bDown = true;
		return;
	}
	actionState = ActionNone;
	// alvo da saída na execução
	node = running ? valueAt(x, y) : NULL;
	if(node != NULL)
		return toggle(node);
	selectNode(NULL);
	NeuronNode *bias = biasAt(x, y);
	if(bias != NULL)
	{
		selectEdge(NULL);
		if(bias == biasSelected)
			request = Request{ Request::EditBias, bias, NULL };
		selectBias(bias);
		return;
	}
	selectBias(NULL);
	Edge *edge = edgeAt(x, y);
	if(edge != NULL && edge == edgeSelected && edge->IsInLabel(Point(x, y)))
		request = Request{ Request::EditWeight, NULL, edge };
	selectEdge(edge);
	bDown = true;
}

void NeuralNetwork::mouseDoubleClick(int x, int y)
{
	Node *node = neuronAt(x, y);
	// na execução cada clique na entrada ou na saída alterna o valor
	if(node != NULL && running && !node->is(Node::TypeNeuron))
		return;
	cancelAction();
	if(node != NULL)
	{
		if(node->is(Node::TypeOutput))
			return;
		selectNode(node);
		request = Request{ Request::Menu, node, NULL };
		return;
	}
	NeuronNode *bias = biasAt(x, y);
	if(bias != NULL)
	{
		selectBias(bias);
		request = Request{ Request::EditBias, bias, NULL };
		return;
	}
	Edge *edge = edgeAt(x, y);
	if(edge == NULL)
		return;
	selectEdge(edge);
	request = Request{ Request::EditWeight, NULL, edge };
}

void NeuralNetwork::mouseRightDown(int x, int y)
{
	cancelAction();
	Node *node = neuronAt(x, y);
	// o menu do bias é o do neurônio
	if(node == NULL)
		node = biasAt(x, y);
	if(node != NULL)
	{
		selectEdge(NULL);
		selectBias(NULL);
		selectNode(node);
		request = Request{ Request::Menu, node, NULL };
		return;
	}
	Edge *edge = edgeAt(x, y);
	selectNode(NULL);
	selectBias(NULL);
	selectEdge(edge);
	if(edge != NULL)
		request = Request{ Request::Menu, NULL, edge };
}

void NeuralNetwork::toggle(Node *node)
{
	if(node->is(Node::TypeInput))
		((InputNode*)node)->toggleValue();
	else if(node->is(Node::TypeOutput))
		((OutputNode*)node)->toggleTarget();
	execution.clearChanged();
	execution.execute();
}

NeuralNetwork::Request NeuralNetwork::takeRequest()
{
	Request result = request;
	request = Request{ Request::None, NULL, NULL };
	return result;
}

void NeuralNetwork::setRunning(bool running)
{
	this->running = running;
	cancelAction();
	execution.clearChanged();
	if(running)
		execution.execute();
}

bool NeuralNetwork::isRunning()
{
	return running;
}

Execution::TrainResult NeuralNetwork::train()
{
	Execution::TrainResult result = execution.train();
	if(result.iterations > 0)
		modified = true;
	return result;
}

bool NeuralNetwork::isAdding()
{
	return actionState == ActionAdd;
}

bool NeuralNetwork::isMoving()
{
	return dragging;
}

bool NeuralNetwork::isConnecting()
{
	return actionState == ActionConnect;
}

bool NeuralNetwork::hasCycle()
{
	return execution.hasCycle();
}

double NeuralNetwork::getError()
{
	return execution.getError();
}

bool NeuralNetwork::isOnTarget()
{
	return execution.isOnTarget();
}

bool NeuralNetwork::isModified()
{
	return modified;
}

Edge* NeuralNetwork::getSelectedEdge()
{
	return edgeSelected;
}

Node* NeuralNetwork::getSelectedNode()
{
	return nodeSelected;
}

NeuronNode* NeuralNetwork::getSelectedBias()
{
	return biasSelected;
}

void NeuralNetwork::saveToFile(const std::string& fileName)
{
	std::ofstream file(pathFromUtf8(fileName), std::ios::binary | std::ios::trunc);
	std::vector<Node *>::iterator it;
	std::vector<Edge *>::iterator ite;
	std::map<Node*,int> nodeMap;
	int intVal;
	int i = 1;

	if(!file)
		throw std::runtime_error("Unable to create " + fileName + ".");
	// 1.1 enquanto não há bias, para continuar abrindo na versão Windows
	bool hasBias = false;
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		Node *node = *it;
		if((node->is(Node::TypeNeuron) && ((NeuronNode*)node)->getBias() != 0) ||
			(node->is(Node::TypeInput) && ((InputNode*)node)->isBias()))
			hasBias = true;
	}
	file.write(hasBias ? RN_SIGN_BIAS : RN_SIGN, 3); // sign
	writeUInt32(file, (uint32_t)neurons.size()); // neuron count
	writeUInt32(file, (uint32_t)edges.size()); // edge count
	// save each neuron
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		Node *node = *it;
		intVal = node->getType();
		writeInt32(file, intVal); // type, 1 -> Neuron, 2 -> Input, 3 ->Output
		if(node->is(Node::TypeNeuron))
			intVal = ((NeuronNode*)node)->getFunctionType();
		else if(node->is(Node::TypeInput) && ((InputNode*)node)->isBias())
			intVal = 1;
		else
			intVal = 0;
		writeInt32(file, intVal); // activation function, 1 on a bias input
		writeInt32(file, node->getPosition().x); // x position
		writeInt32(file, node->getPosition().y); // y position
		if(hasBias)
		{
			double bias = 0;
			if(node->is(Node::TypeNeuron))
				bias = ((NeuronNode*)node)->getBias();
			writeDouble(file, bias); // neuron bias
		}
		nodeMap[node] = i;
		i++;
	}
	for(ite = edges.begin(); ite != edges.end(); ite++)
	{
		Edge *edge = *ite;
		writeInt32(file, nodeMap[edge->getFirst()]); // first node id
		writeInt32(file, nodeMap[edge->getSecond()]); // second node id
		writeDouble(file, edge->getWeight()); // connection weight
	}
	file.close();
	if(!file)
		throw std::runtime_error("Unable to write " + fileName + ".");
	modified = false;
}

void NeuralNetwork::loadFromFile(const std::string& fileName)
{
	struct NodeData
	{
		int type, function, x, y;
		double bias;
	};
	struct EdgeData
	{
		int first, second;
		double weight;
	};
	std::ifstream file(pathFromUtf8(fileName), std::ios::binary);
	std::vector<NodeData> nodes;
	std::vector<EdgeData> links;
	uint32_t neuronCount;
	uint32_t edgeCount;
	unsigned char ver[4];
	uint32_t i;

	if(!file)
		throw std::runtime_error("Unable to open " + fileName + ".");
	ver[3] = '\0';
	readBytes(file, ver, 3); // sign
	bool hasBias = strcmp((const char*)ver, RN_SIGN_BIAS) == 0;
	if(!hasBias && strcmp((const char*)ver, RN_SIGN) != 0)
	{
		if(!isprint(ver[0]) || !isprint(ver[1]) || !isprint(ver[2]))
			throw std::runtime_error("Not a neural network file.");
		throw std::runtime_error(std::string("Version ") + (const char*)ver +
			" not supported, required " RN_SIGN " or " RN_SIGN_BIAS ".");
	}
	neuronCount = readUInt32(file); // neuron count
	edgeCount = readUInt32(file); // edge count
	// lê o arquivo inteiro antes de alterar a rede, um arquivo inválido
	// mantém a rede atual
	for(i = 0; i < neuronCount; i++)
	{
		NodeData node;
		node.type = readInt32(file); // type, 1 -> Neuron, 2 -> Input, 3 ->Output
		node.function = readInt32(file); // activation function
		node.x = readInt32(file); // x position
		node.y = readInt32(file); // y position
		node.bias = hasBias ? readDouble(file) : 0; // neuron bias
		if(node.type == Node::TypeNeuron &&
			(node.function < 0 || node.function >= NeuronNode::FunctionCount))
			throw std::runtime_error("Invalid activation function.");
		// a 1.1 não marca entradas de bias
		if(node.type == Node::TypeInput && !hasBias)
			node.function = 0;
		if(node.type == Node::TypeInput && node.function != 0 && node.function != 1)
			throw std::runtime_error("Invalid input kind.");
		nodes.push_back(node);
	}
	for(i = 0; i < edgeCount; i++)
	{
		EdgeData edge;
		edge.first = readInt32(file); // first node id
		edge.second = readInt32(file); // second node id
		edge.weight = readDouble(file); // connection weight
		if(edge.first < 1 || edge.first > (int)nodes.size() ||
			edge.second < 1 || edge.second > (int)nodes.size())
			throw std::runtime_error("Invalid node id.");
		links.push_back(edge);
	}
	clear();
	for(size_t n = 0; n < nodes.size(); n++)
	{
		Node *node;
		if(nodes[n].type == Node::TypeNeuron)
		{
			NeuronNode *neuron = new NeuronNode();
			neuron->setFunctionType(nodes[n].function);
			neuron->setBias(nodes[n].bias);
			node = neuron;
		}
		else if(nodes[n].type == Node::TypeInput)
		{
			InputNode *input = new InputNode();
			input->setBias(nodes[n].function == 1);
			node = input;
		}
		else
			node = new OutputNode();
		node->setPosition(nodes[n].x, nodes[n].y);
		neurons.push_back(node);
	}
	for(size_t e = 0; e < links.size(); e++)
	{
		Node *first = neurons[links[e].first - 1];
		Node *second = neurons[links[e].second - 1];
		if(canConnect(first, second))
			edges.push_back(new Edge(first, second, links[e].weight));
	}
	modified = false;
	if(running)
		execution.execute();
}
