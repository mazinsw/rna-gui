#include "NeuralNetwork.h"
#include "Platform.h"
#include "Shapes.h"
#include "Widgets.h"
#include <cmath>
#include <rlgl.h>
#include <algorithm>
#include <functional>
#include <map>
#include <stdexcept>
#include <stdio.h>
// pontos da grade a cada GRID_SIZE pixels, o primeiro em GRID_OFFSET
#define GRID_SIZE 20
#define GRID_OFFSET 10
// distância na tela em que o nó arrastado entra na camada ou na linha de
// outro nó
#define SNAP_DISTANCE 8
// distância entre camadas e entre os nós de uma camada ao reorganizar
#define ARRANGE_MIN_LAYER_SPACING 140
#define ARRANGE_MAX_LAYER_SPACING 200
#define ARRANGE_MIN_NODE_SPACING 120
#define ARRANGE_MAX_NODE_SPACING 160
// distância entre as entradas de uma imagem, em quadrado como os pixels
#define ARRANGE_PIXEL_SPACING 40
// nós a menos disto no x ficam na mesma camada
#define LAYER_TOLERANCE 30
// limites do zoom e passo de cada volta da roda do mouse
#define MIN_ZOOM 0.25f
#define MAX_ZOOM 4.0f
#define ZOOM_STEP 1.1f
// menor distância na tela entre os pontos da grade
#define MIN_GRID_SPACING 12
// com mais conexões que isto os rótulos de peso cobririam a rede, só o da
// conexão selecionada aparece
#define MAX_WEIGHT_LABELS 100

NeuralNetwork::NeuralNetwork(): execution(neurons, edges), random(std::random_device()())
{
	bDown = false;
	bMoving = false;
	dragging = false;
	snapX = snapY = SnapNone;
	zoom = 1;
	pan = Vector2{ 0, 0 };
	panning = false;
	panX = panY = 0;
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
	showingImage = false;
	pressX = pressY = 0;
	request = Request{ Request::None, NULL, NULL };
	layerType = Node::TypeNeuron;
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

Node *NeuralNetwork::newNode(Node::Type type)
{
	if(type == Node::TypeInput)
		return new InputNode();
	if(type == Node::TypeOutput)
		return new OutputNode();
	return newNeuron();
}

void NeuralNetwork::addNeuron(Rectangle area)
{
	addToColumn(Node::TypeNeuron, area);
}

void NeuralNetwork::addInput(Rectangle area)
{
	addToColumn(Node::TypeInput, area);
}

void NeuralNetwork::addOutput(Rectangle area)
{
	addToColumn(Node::TypeOutput, area);
}

void NeuralNetwork::paint(Rectangle bounds)
{
	DrawRectangleRec(bounds, ui::toColor(ui::CanvasColor));
	BeginScissorMode((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height);
	// grade de pontos, onde os nós se alinham. Com o zoom afastado fica um a
	// cada 2, 4..., para os pontos não se juntarem
	Color grid = ui::toColor(ui::GridColor);
	float spacing = GRID_SIZE * zoom;
	while(spacing < MIN_GRID_SPACING)
		spacing *= 2;
	Vector2 first = toScreen(Vector2{ GRID_OFFSET, GRID_OFFSET });
	float left = bounds.x + first.x - floorf(first.x / spacing) * spacing;
	float top = bounds.y + first.y - floorf(first.y / spacing) * spacing;
	for(float gy = top; gy < bounds.y + bounds.height; gy += spacing)
	{
		for(float gx = left; gx < bounds.x + bounds.width; gx += spacing)
			DrawRectangleRec(Rectangle{ gx - 0.75f, gy - 0.75f, 1.5f, 1.5f }, grid);
	}
	drawGuides(bounds);
	// desenha a rede com o zoom e o deslocamento da vista. Sem BeginMode2D: no
	// raylib 6 ele descarta a escala da tela Retina e a rede sairia com metade
	// do tamanho, longe de onde o mouse a encontra
	Vector2 offset = { bounds.x + pan.x, bounds.y + pan.y };
	shapes::setZoom(zoom);
	Node::setView(zoom, offset);
	rlPushMatrix();
	rlTranslatef(offset.x, offset.y, 0);
	rlScalef(zoom, zoom, 1);
	draw();
	rlPopMatrix();
	Node::setView(1, Vector2{ 0, 0 });
	shapes::setZoom(1);
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

void NeuralNetwork::drawGuides(Rectangle bounds)
{
	Node *node = NULL;
	if(dragging)
		node = nodeMoving;
	else if(actionState == ActionLayer && !layerNodes.empty())
		node = layerNodes[0];
	if(node == NULL)
		return;
	// por baixo das conexões e dos nós: azul na camada ou na linha de outro
	// nó, tracejada no ponto da grade
	Vector2 center = toScreen(node->getCenter());
	center = Vector2{ bounds.x + center.x, bounds.y + center.y };
	Color layer = ui::toColor(ui::AccentColor);
	Color grid = ui::toColor(ui::FaintColor);
	Vector2 top = { center.x, bounds.y }, bottom = { center.x, bounds.y + bounds.height };
	Vector2 left = { bounds.x, center.y }, right = { bounds.x + bounds.width, center.y };
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
	float distanceX = SNAP_DISTANCE / zoom + 1, distanceY = distanceX;
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

Point NeuralNetwork::toWorld(int x, int y)
{
	return Point((int)floorf((x - pan.x) / zoom + 0.5f), (int)floorf((y - pan.y) / zoom + 0.5f));
}

Vector2 NeuralNetwork::toScreen(Vector2 point)
{
	return Vector2{ point.x * zoom + pan.x, point.y * zoom + pan.y };
}

Rectangle NeuralNetwork::toScreen(Rectangle rect)
{
	Vector2 corner = toScreen(Vector2{ rect.x, rect.y });
	return Rectangle{ corner.x, corner.y, rect.width * zoom, rect.height * zoom };
}

void NeuralNetwork::zoomAt(int x, int y, float wheel)
{
	float next = zoom * powf(ZOOM_STEP, wheel);
	next = next < MIN_ZOOM ? MIN_ZOOM : next > MAX_ZOOM ? MAX_ZOOM : next;
	pan.x = x - (x - pan.x) * next / zoom;
	pan.y = y - (y - pan.y) * next / zoom;
	zoom = next;
}

void NeuralNetwork::startPanning(int x, int y)
{
	panning = true;
	panX = x;
	panY = y;
}

void NeuralNetwork::startPan(int x, int y)
{
	// não interrompe o arraste de um nó
	if(!bDown)
		startPanning(x, y);
}

void NeuralNetwork::endPan()
{
	panning = false;
}

void NeuralNetwork::fitView(Rectangle area)
{
	zoom = 1;
	pan = Vector2{ 0, 0 };
	if(neurons.empty())
		return;
	Point low = neurons[0]->getPosition(), high = low;
	std::vector<Node *>::iterator it;
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		const Point& p = (*it)->getPosition();
		low = Point(std::min(low.x, p.x), std::min(low.y, p.y));
		high = Point(std::max(high.x, p.x), std::max(high.y, p.y));
	}
	if(high.x - low.x > area.width)
		zoom = area.width / (high.x - low.x);
	if((high.y - low.y) * zoom > area.height)
		zoom = area.height / (high.y - low.y);
	if(zoom < MIN_ZOOM)
		zoom = MIN_ZOOM;
	// no pixel inteiro, para as linhas finas não borrarem
	pan.x = roundf(area.x + area.width / 2 - (low.x + high.x) / 2.0f * zoom);
	pan.y = roundf(area.y + area.height / 2 - (low.y + high.y) / 2.0f * zoom);
}

bool NeuralNetwork::fitsIn(Rectangle area)
{
	std::vector<Node *>::iterator it;
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		Vector2 p = toScreen((*it)->getCenter());
		if(p.x < area.x || p.y < area.y || p.x > area.x + area.width ||
			p.y > area.y + area.height)
			return false;
	}
	return true;
}

float NeuralNetwork::getZoom()
{
	return zoom;
}

Vector2 NeuralNetwork::getPan()
{
	return pan;
}

void NeuralNetwork::setView(float zoom, Vector2 pan)
{
	this->zoom = zoom < MIN_ZOOM ? MIN_ZOOM : zoom > MAX_ZOOM ? MAX_ZOOM : zoom;
	this->pan = pan;
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
	bool drawingLayer = actionState == ActionLayer && !layerNodes.empty();
	Layer layer;
	if(drawingLayer)
		layer = layerAt(layerStart.x);
	// a camada nova arrastada entre duas esconde as conexões diretas que vai
	// remover
	std::vector<Edge *> visible;
	std::vector<Edge *>::iterator ite;
	for(ite = edges.begin(); ite != edges.end(); ite++)
	{
		if(drawingLayer && layer.nodes.empty() &&
			std::find(layer.prev.begin(), layer.prev.end(), (*ite)->getFirst()) != layer.prev.end() &&
			std::find(layer.next.begin(), layer.next.end(), (*ite)->getSecond()) != layer.next.end())
			continue;
		visible.push_back(*ite);
	}
	for(ite = visible.begin(); ite != visible.end(); ite++)
		(*ite)->drawLine();
	if(!bMoving && actionState == ActionConnect)
		shapes::line(edgeAdd->getFirst()->getCenter(), nodeAdd->getCenter(), 2,
			ui::toColor(ui::FaintColor));
	std::vector<Node *>::iterator it;
	if(drawingLayer)
	{
		// conexões que a camada arrastada vai ganhar
		Color faint = ui::toColor(ui::FaintColor);
		for(it = layerNodes.begin(); it != layerNodes.end(); it++)
		{
			std::vector<Node *>::iterator itl;
			for(itl = layer.prev.begin(); itl != layer.prev.end(); itl++)
			{
				if(canLink(*itl, *it))
					shapes::line((*itl)->getCenter(), (*it)->getCenter(), 1.5f, faint);
			}
			for(itl = layer.next.begin(); itl != layer.next.end(); itl++)
			{
				if(canLink(*it, *itl))
					shapes::line((*it)->getCenter(), (*itl)->getCenter(), 1.5f, faint);
			}
		}
	}
	bool labels = edges.size() <= MAX_WEIGHT_LABELS;
	for(ite = edges.begin(); ite != edges.end(); ite++)
		(*ite)->hideLabel();
	for(ite = visible.begin(); ite != visible.end(); ite++)
	{
		if(labels || (*ite)->isSelected())
			(*ite)->drawLabel();
	}
	for(it = neurons.begin(); it != neurons.end(); it++)
		(*it)->draw(running);
	for(it = layerNodes.begin(); it != layerNodes.end(); it++)
		(*it)->draw(false);
	if(running && showingImage)
		drawClasses();
}

void NeuralNetwork::drawClasses()
{
	std::vector<Node *> outputs = readingOrder(Node::TypeOutput);
	int recognized = getRecognized();
	float size = ui::LabelFontSize;
	for(size_t i = 0; i < outputs.size(); i++)
	{
		Vector2 center = outputs[i]->getCenter();
		std::string text = std::to_string(i);
		float width = Node::measureText(text, size).x + 16;
		Vector2 middle = { center.x + 26 + width / 2, center.y };
		Color color = ui::toColor(ui::MutedColor);
		if((int)i == recognized)
		{
			Rectangle box = { middle.x - width / 2, middle.y - 12, width, 24 };
			Node::drawPill(box, ui::toColor(ui::AccentColor), ui::toColor(ui::AccentColor));
			color = WHITE;
		}
		Node::drawTextCentered(text, middle, size, color);
	}
}

std::vector<Node *> NeuralNetwork::readingOrder(Node::Type type)
{
	std::vector<Node *> found;
	std::vector<std::pair<int, int> > positions;
	for(Node *node : neurons)
	{
		if(!node->is(type) || (type == Node::TypeInput && ((InputNode*)node)->isBias()))
			continue;
		found.push_back(node);
		positions.push_back(std::make_pair(node->getPosition().x, node->getPosition().y));
	}
	std::vector<Node *> result;
	for(int i : Network::readingOrder(positions))
		result.push_back(found[i]);
	return result;
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
	return Network::canConnect(first->getType(), second->getType());
}

bool NeuralNetwork::canLink(Node *first, Node *second)
{
	return !first->is(Node::TypeOutput) && !second->is(Node::TypeInput) &&
		canConnect(first, second);
}

void NeuralNetwork::connect(Node *first, Node *second)
{
	if(!canConnect(first, second))
		return;
	// a conexão com a saída repassa o valor do neurônio, as demais começam
	// com peso aleatório
	double weight = second->is(Node::TypeOutput) ? 1 : randomWeight();
	edges.push_back(new Edge(first, second, weight));
	modified = true;
}

void NeuralNetwork::addEdge(Node *node)
{
	connect(edgeAdd->getFirst(), node);
}

void NeuralNetwork::connectAll(const std::vector<Node *>& sources,
	const std::vector<Node *>& targets)
{
	for(size_t s = 0; s < sources.size(); s++)
	{
		for(size_t t = 0; t < targets.size(); t++)
		{
			if(canLink(sources[s], targets[t]))
				connect(sources[s], targets[t]);
		}
	}
}

void NeuralNetwork::disconnectAll(const std::vector<Node *>& sources,
	const std::vector<Node *>& targets)
{
	std::vector<Edge *>::iterator ite = edges.begin();
	while(ite != edges.end())
	{
		Edge *edge = *ite;
		if(std::find(sources.begin(), sources.end(), edge->getFirst()) == sources.end() ||
			std::find(targets.begin(), targets.end(), edge->getSecond()) == targets.end())
		{
			ite++;
			continue;
		}
		if(edgeSelected == edge)
			selectEdge(NULL);
		delete edge;
		ite = edges.erase(ite);
		modified = true;
	}
}

/**
 * x da camada, o do nó mais à esquerda
 */
static int columnX(const std::vector<Node *>& column)
{
	int x = column[0]->getPosition().x;
	for(size_t n = 1; n < column.size(); n++)
		x = std::min(x, column[n]->getPosition().x);
	return x;
}

static bool hasType(const std::vector<Node *>& nodes, Node::Type type)
{
	for(size_t n = 0; n < nodes.size(); n++)
	{
		if(nodes[n]->is(type))
			return true;
	}
	return false;
}

std::vector<std::vector<Node *> > NeuralNetwork::columns()
{
	std::vector<Node *> sorted = neurons;
	std::stable_sort(sorted.begin(), sorted.end(), [](Node *a, Node *b)
	{
		return a->getPosition().x < b->getPosition().x;
	});
	// cada camada junta os nós a menos da tolerância do mais à esquerda
	std::vector<std::vector<Node *> > result;
	std::vector<Node *>::iterator it;
	for(it = sorted.begin(); it != sorted.end(); it++)
	{
		if(result.empty() ||
			(*it)->getPosition().x - result.back()[0]->getPosition().x > LAYER_TOLERANCE)
			result.push_back(std::vector<Node *>());
		result.back().push_back(*it);
	}
	for(size_t c = 0; c < result.size(); c++)
	{
		std::stable_sort(result[c].begin(), result[c].end(), [](Node *a, Node *b)
		{
			return a->getPosition().y < b->getPosition().y;
		});
	}
	return result;
}

NeuralNetwork::Layer NeuralNetwork::layerAt(int x)
{
	Layer layer;
	std::vector<std::vector<Node *> > all = columns();
	// a camada mais próxima dentro da tolerância
	int found = -1;
	int distance = LAYER_TOLERANCE + 1;
	for(size_t c = 0; c < all.size(); c++)
	{
		if(abs(columnX(all[c]) - x) >= distance)
			continue;
		distance = abs(columnX(all[c]) - x);
		found = (int)c;
	}
	if(found >= 0)
	{
		layer.nodes = all[found];
		if(found > 0)
			layer.prev = all[found - 1];
		if(found + 1 < (int)all.size())
			layer.next = all[found + 1];
		return layer;
	}
	// sem camada no x, as vizinhas são as de antes e de depois dele
	for(size_t c = 0; c < all.size(); c++)
	{
		if(columnX(all[c]) < x)
			layer.prev = all[c];
		else if(layer.next.empty())
			layer.next = all[c];
	}
	return layer;
}

void NeuralNetwork::addToLayer(const std::vector<Node *>& nodes)
{
	Layer layer = layerAt(nodes[0]->getPosition().x);
	// a camada nova fica no caminho entre as vizinhas
	if(layer.nodes.empty())
		disconnectAll(layer.prev, layer.next);
	neurons.insert(neurons.end(), nodes.begin(), nodes.end());
	// as amostras têm um valor por entrada e um alvo por saída
	if(!nodes[0]->is(Node::TypeNeuron))
		execution.forgetSamples();
	connectAll(layer.prev, nodes);
	connectAll(nodes, layer.next);
	modified = true;
}

/**
 * Espaçamento entre os nós da camada: a distância mais comum, na grade
 */
static int columnSpacing(const std::vector<Node *>& column)
{
	if(column.size() < 2)
		return ARRANGE_MIN_NODE_SPACING;
	std::vector<int> gaps;
	for(size_t n = 1; n < column.size(); n++)
		gaps.push_back(column[n]->getPosition().y - column[n - 1]->getPosition().y);
	std::sort(gaps.begin(), gaps.end());
	int spacing = (int)roundf(gaps[gaps.size() / 2] / (float)GRID_SIZE) * GRID_SIZE;
	// nós empilhados ganham espaço para não se sobreporem
	return std::max(spacing, ARRANGE_MIN_NODE_SPACING / 2);
}

void NeuralNetwork::addToColumn(Node::Type type, Rectangle area)
{
	cancelAction();
	Node *node = newNode(type);
	std::vector<Node *> added(1, node);
	std::vector<std::vector<Node *> > all = columns();
	if(all.empty())
	{
		// o primeiro nó fica no centro da área
		Point p = toWorld((int)(area.x + area.width / 2), (int)(area.y + area.height / 2));
		node->setPosition(snapToGrid(p.x), snapToGrid(p.y));
		addToLayer(added);
		selectNode(node);
		return;
	}
	// centro vertical da rede antes do nó novo
	int top = neurons[0]->getPosition().y, bottom = top;
	std::vector<Node *>::iterator it;
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		top = std::min(top, (*it)->getPosition().y);
		bottom = std::max(bottom, (*it)->getPosition().y);
	}
	int center = (top + bottom) / 2;
	// o neurônio vai na camada do selecionado; sem ele, como a entrada e a
	// saída, na camada do tipo: a primeira com entradas, a última com os demais
	int target = -1;
	bool neuronSelected = type == Node::TypeNeuron && nodeSelected != NULL &&
		nodeSelected->is(Node::TypeNeuron);
	for(size_t c = 0; c < all.size(); c++)
	{
		bool inColumn = std::find(all[c].begin(), all[c].end(), nodeSelected) != all[c].end();
		if(neuronSelected ? inColumn : (hasType(all[c], type) &&
			(target < 0 || type != Node::TypeInput)))
			target = (int)c;
	}
	if(target >= 0)
	{
		// abaixo do último, no x do selecionado se for desta camada
		std::vector<Node *> column = all[target];
		int spacing = columnSpacing(column);
		Node *last = column.back();
		int x = last->getPosition().x;
		if(std::find(column.begin(), column.end(), nodeSelected) != column.end())
			x = nodeSelected->getPosition().x;
		node->setPosition(x, last->getPosition().y + spacing);
		addToLayer(added);
		// a camada com espaçamento igual, centralizada na rede
		column.push_back(node);
		int first = snapToGrid(center - spacing * ((int)column.size() - 1) / 2);
		for(size_t n = 0; n < column.size(); n++)
			column[n]->setPosition(column[n]->getPosition().x, first + spacing * (int)n);
		selectNode(node);
		return;
	}
	// sem a camada do tipo, cria uma: a de entrada antes da primeira, a de
	// saída depois da última e a de neurônios depois das entradas
	int x;
	if(type == Node::TypeInput)
		x = columnX(all.front()) - ARRANGE_MAX_LAYER_SPACING;
	else if(type == Node::TypeOutput)
		x = columnX(all.back()) + ARRANGE_MAX_LAYER_SPACING;
	else
	{
		int inputs = -1;
		for(size_t c = 0; c < all.size(); c++)
		{
			if(hasType(all[c], Node::TypeInput))
				inputs = (int)c;
		}
		if(inputs < 0)
			x = columnX(all.front()) - ARRANGE_MAX_LAYER_SPACING;
		else if(inputs + 1 == (int)all.size())
			x = columnX(all[inputs]) + ARRANGE_MAX_LAYER_SPACING;
		else
		{
			int left = columnX(all[inputs]), right = columnX(all[inputs + 1]);
			x = snapToGrid((left + right) / 2);
			if(right - left < 2 * ARRANGE_MIN_LAYER_SPACING)
			{
				// sem espaço entre as camadas, as seguintes vão para a direita
				x = left + ARRANGE_MAX_LAYER_SPACING;
				for(size_t c = inputs + 1; c < all.size(); c++)
				{
					for(it = all[c].begin(); it != all[c].end(); it++)
					{
						const Point& p = (*it)->getPosition();
						(*it)->setPosition(p.x + ARRANGE_MAX_LAYER_SPACING, p.y);
					}
				}
			}
		}
	}
	node->setPosition(x, snapToGrid(center));
	addToLayer(added);
	selectNode(node);
}

void NeuralNetwork::startLayer(int x, int y)
{
	cancelLayer();
	actionState = ActionLayer;
	layerStart = snapPosition(NULL, x, y);
	Layer layer = layerAt(layerStart.x);
	if(layer.nodes.empty())
	{
		// a rede começa pelas entradas, depois delas vêm os neurônios
		layerType = hasType(neurons, Node::TypeInput) ? Node::TypeNeuron : Node::TypeInput;
		return;
	}
	// sobre uma camada os nós entram nela, com o tipo dela
	if(snapX != SnapLayer)
		layerStart.x = columnX(layer.nodes);
	snapX = SnapLayer;
	layerType = layer.nodes[0]->getType();
	std::vector<Node *>::iterator it;
	for(it = layer.nodes.begin(); it != layer.nodes.end(); it++)
	{
		if(!(*it)->is(layerType))
			layerType = Node::TypeNeuron;
	}
}

void NeuralNetwork::updateLayer(int x, int y)
{
	// o tremor do clique não cria nós
	if(layerNodes.empty() && abs(x - pressX) <= 3 && abs(y - pressY) <= 3)
		return;
	// um nó a mais a cada espaçamento arrastado, para cima ou para baixo
	int distance = toWorld(x, y).y - layerStart.y;
	int direction = distance < 0 ? -1 : 1;
	size_t count = 1 + (size_t)floorf(abs(distance) / (float)ARRANGE_MIN_NODE_SPACING + 0.5f);
	while(layerNodes.size() < count)
		layerNodes.push_back(newNode(layerType));
	while(layerNodes.size() > count)
	{
		delete layerNodes.back();
		layerNodes.pop_back();
	}
	for(size_t n = 0; n < layerNodes.size(); n++)
		layerNodes[n]->setPosition(layerStart.x,
			layerStart.y + direction * ARRANGE_MIN_NODE_SPACING * (int)n);
}

void NeuralNetwork::finishLayer()
{
	actionState = ActionNone;
	if(layerNodes.empty())
		return;
	addToLayer(layerNodes);
	selectNode(layerNodes.back());
	layerNodes.clear();
}

void NeuralNetwork::cancelLayer()
{
	std::vector<Node *>::iterator it;
	for(it = layerNodes.begin(); it != layerNodes.end(); it++)
		delete *it;
	layerNodes.clear();
	if(actionState == ActionLayer)
		actionState = ActionNone;
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
	if(!node->is(Node::TypeNeuron))
		execution.forgetSamples();
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
	cancelLayer();
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
	cancelLayer();
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
 * Maior espaçamento entre min e max, múltiplo da grade, para count nós
 * caberem em size
 */
static int arrangeSpacing(int count, float size, int min, int max)
{
	if(count < 2)
		return 0;
	int spacing = (int)(size / (count - 1)) / GRID_SIZE * GRID_SIZE;
	if(spacing > max)
		spacing = max;
	return spacing < min ? min : spacing;
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
	// as entradas de uma imagem, sozinhas na primeira camada, ficam em quadrado
	// na ordem de leitura dos pixels
	int side = getImageSide();
	std::vector<Node *> image;
	if(side > 0 && columns[0].size() == (size_t)(side * side))
		image = readingOrder(Node::TypeInput);
	int gridSize = image.empty() ? 0 : ARRANGE_PIXEL_SPACING * (side - 1);
	int spacingX = arrangeSpacing((int)columns.size(), area.width - gridSize,
		ARRANGE_MIN_LAYER_SPACING, ARRANGE_MAX_LAYER_SPACING);
	int left = snapToGrid((int)(area.x + (area.width - gridSize -
		spacingX * ((int)columns.size() - 1)) / 2));
	std::map<Node *, Point> positions;
	if(!image.empty())
	{
		int top = snapToGrid((int)(area.y + (area.height - gridSize) / 2));
		for(size_t i = 0; i < image.size(); i++)
			positions[image[i]] = Point(left + ARRANGE_PIXEL_SPACING * (int)(i % side),
				top + ARRANGE_PIXEL_SPACING * (int)(i / side));
		left += gridSize;
	}
	for(size_t c = image.empty() ? 0 : 1; c < columns.size(); c++)
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
		int spacingY = arrangeSpacing((int)column.size(), area.height, ARRANGE_MIN_NODE_SPACING,
			ARRANGE_MAX_NODE_SPACING);
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
	execution.forgetSamples();
	notifyDataChanged();
}

void NeuralNetwork::clear()
{
	execution.forgetSamples();
	cancelLayer();
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
	showingImage = false;
	bDown = false;
	bMoving = false;
	dragging = false;
	actionState = ActionNone;
	request = Request{ Request::None, NULL, NULL };
	// a rede nova abre sem zoom e sem deslocamento
	zoom = 1;
	pan = Vector2{ 0, 0 };
	panning = false;
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
	cancelLayer();
	actionState = ActionNone;
	nodeToggle = NULL;
	bMoving = false;
	dragging = false;
	panning = false;
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
	// o botão do meio já arrasta a vista
	if(panning)
		return;
	int screenX = pressX = x;
	int screenY = pressY = y;
	Point p = toWorld(x, y);
	x = downX = p.x;
	y = downY = p.y;
	nodeToggle = NULL;
	dragging = false;
	Node *node = neuronAt(x, y);
	if(node != NULL)
	{
		pressNode(node, x, y);
		bDown = true;
		return;
	}
	actionState = ActionNone;
	// Shift fora dos nós arrasta uma camada nova, mesmo sobre conexões e bias
	if(!running && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))
	{
		selectNode(NULL);
		selectEdge(NULL);
		selectBias(NULL);
		startLayer(x, y);
		bDown = true;
		return;
	}
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
	// no fundo vazio arrasta a vista
	if(edge == NULL)
		startPanning(screenX, screenY);
}

void NeuralNetwork::mouseDoubleClick(int x, int y)
{
	Point p = toWorld(x, y);
	x = p.x;
	y = p.y;
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
	Point p = toWorld(x, y);
	x = p.x;
	y = p.y;
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
	{
		((InputNode*)node)->toggleValue();
		showingImage = false;
	}
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
	showingImage = showingImage && running;
	cancelAction();
	execution.clearChanged();
	if(running)
		execution.execute();
}

bool NeuralNetwork::isRunning()
{
	return running;
}

Training::Result NeuralNetwork::train(bool withScreen)
{
	Training::Result result = execution.train(withScreen);
	if(result.changed || result.samplesChanged)
		modified = true;
	return result;
}

int NeuralNetwork::getSampleCount()
{
	return execution.getSampleCount();
}

void NeuralNetwork::forgetSamples()
{
	if(execution.getSampleCount() > 0)
		modified = true;
	execution.forgetSamples();
}

void NeuralNetwork::randomizeWeights()
{
	std::vector<bool> trainable = execution.isTrainable();
	for(size_t i = 0; i < edges.size(); i++)
	{
		if(trainable[i])
			edges[i]->setWeight(randomWeight());
	}
	for(Node* node : neurons)
	{
		if(node->is(Node::TypeNeuron))
			((NeuronNode*)node)->setBias(randomWeight());
	}
	notifyDataChanged();
}

bool NeuralNetwork::isDrawingLayer()
{
	return actionState == ActionLayer;
}

bool NeuralNetwork::isEmpty()
{
	return neurons.empty();
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

int NeuralNetwork::getImageSide()
{
	return Network::imageSide((int)readingOrder(Node::TypeInput).size());
}

bool NeuralNetwork::showImage(const std::vector<double>& values, int label)
{
	running = true;
	showingImage = true;
	cancelAction();
	execution.clearChanged();
	return execution.showImage(values, label);
}

bool NeuralNetwork::isShowingImage()
{
	return showingImage;
}

int NeuralNetwork::getRecognized()
{
	std::vector<Node *> outputs = readingOrder(Node::TypeOutput);
	int recognized = -1;
	for(size_t i = 0; i < outputs.size(); i++)
	{
		double value = outputs[i]->getValue();
		if(!std::isfinite(value))
			return -1;
		if(recognized < 0 || value > outputs[recognized]->getValue())
			recognized = (int)i;
	}
	return recognized;
}

double NeuralNetwork::getOutputValue(int output)
{
	return readingOrder(Node::TypeOutput)[output]->getValue();
}

int NeuralNetwork::getExpected()
{
	std::vector<Node *> outputs = readingOrder(Node::TypeOutput);
	int expected = -1;
	for(size_t i = 0; i < outputs.size(); i++)
	{
		if(((OutputNode*)outputs[i])->getTarget() == 0)
			continue;
		if(expected >= 0)
			return -1;
		expected = (int)i;
	}
	return expected;
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
	std::filesystem::path path = pathFromUtf8(fileName);
	execution.getNetwork().save(path);
	execution.saveSamples(path);
	modified = false;
}

int NeuralNetwork::loadSamples(const std::string& fileName)
{
	return execution.loadSamples(pathFromUtf8(fileName));
}

void NeuralNetwork::loadFromFile(const std::string& fileName)
{
	// lê o arquivo inteiro antes de alterar a rede, um arquivo inválido
	// mantém a rede atual
	Network network = Network::load(pathFromUtf8(fileName));
	clear();
	for(const Network::Node& data : network.nodes)
	{
		Node *node;
		if(data.type == Network::Neuron)
		{
			NeuronNode *neuron = new NeuronNode();
			neuron->setFunctionType(data.function);
			neuron->setBias(data.bias);
			node = neuron;
		}
		else if(data.type == Network::Input)
		{
			InputNode *input = new InputNode();
			input->setBias(data.biasInput);
			node = input;
		}
		else
			node = new OutputNode();
		node->setPosition(data.x, data.y);
		neurons.push_back(node);
	}
	for(const Network::Edge& edge : network.edges)
		edges.push_back(new Edge(neurons[edge.source], neurons[edge.target], edge.weight));
	modified = false;
	if(running)
		execution.execute();
}
