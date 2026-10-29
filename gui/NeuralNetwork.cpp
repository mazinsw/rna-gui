#include "NeuralNetwork.h"
#include "Platform.h"
#include "Resources.h"
#include <ctype.h>
#include <fstream>
#include <map>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#define RN_SIGN "1.1"

/*
 * Formato do arquivo, little-endian com tamanhos fixos para ser o mesmo em
 * qualquer plataforma (compatível com o gravado pela versão Windows 32 bits):
 * sign[3] neuronCount:u32 edgeCount:u32
 * neuronCount x (type:i32 function:i32 x:i32 y:i32)
 * edgeCount x (first:i32 second:i32 weight:f64)
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

NeuralNetwork::NeuralNetwork()
{
	neuronImg = loadTextureResource("neuron");
	inputImg = loadTextureResource("input");
	outputImg = loadTextureResource("output");
	funcImgs.push_back(loadTextureResource("funcao_linear"));
	funcImgs.push_back(loadTextureResource("funcao_degree"));
	funcImgs.push_back(loadTextureResource("funcao_bipolar"));
	funcImgs.push_back(loadTextureResource("funcao_logistic"));
	funcImgs.push_back(loadTextureResource("funcao_hiperbolic"));
	bDown = false;
	bMoving = false;
	x = y = downX = downY = 0;
	nodeMoving = NULL;
	nodeSelected = NULL;
	edgeSelected = NULL;
	nodeAdd = new NeuronNode(neuronImg, funcImgs);
	edgeAdd = new Edge(NULL, nodeAdd, 0);
	actionState = ActionNone;
	modified = false;
}

NeuralNetwork::~NeuralNetwork()
{
	clear();
	std::vector<Texture2D>::iterator itm;
	for(itm = funcImgs.begin(); itm != funcImgs.end(); itm++)
		UnloadTexture(*itm);
	delete edgeAdd;
	delete nodeAdd;
	UnloadTexture(outputImg);
	UnloadTexture(inputImg);
	UnloadTexture(neuronImg);
}

void NeuralNetwork::addNeuron()
{
	actionState = ActionAdd;
	delete nodeAdd;
	nodeAdd = new NeuronNode(neuronImg, funcImgs);
	edgeAdd->setSecond(nodeAdd);
}

void NeuralNetwork::addInput()
{
	actionState = ActionAdd;
	delete nodeAdd;
	nodeAdd = new InputNode(inputImg);
	edgeAdd->setSecond(nodeAdd);
}

void NeuralNetwork::addOutput()
{
	actionState = ActionAdd;
	delete nodeAdd;
	nodeAdd = new OutputNode(outputImg);
	edgeAdd->setSecond(nodeAdd);
}

void NeuralNetwork::addNeuron(int x, int y)
{
	Node *node = nodeAdd;
	node->setPosition(x, y);
	neurons.push_back(node);
	nodeAdd = new NeuronNode(neuronImg, funcImgs);
	edgeAdd->setSecond(nodeAdd);
	modified = true;
}

void NeuralNetwork::paint(Rectangle bounds)
{
	DrawRectangleRec(bounds, WHITE);
	DrawRectangleLinesEx(bounds, 1, GRAY);
	BeginScissorMode((int)bounds.x + 1, (int)bounds.y + 1, (int)bounds.width - 2,
		(int)bounds.height - 2);
	// desenha com a origem no canto da área de edição
	Camera2D camera = { { bounds.x, bounds.y }, { 0, 0 }, 0, 1 };
	BeginMode2D(camera);
	draw();
	EndMode2D();
	EndScissorMode();
}

void NeuralNetwork::draw()
{
	std::vector<Edge *>::iterator ite;
	for(ite = edges.begin(); ite != edges.end(); ite++)
	{
		Edge *edge = *ite;
		edge->draw();
	}
	if(!bMoving && actionState == ActionConnect)
	{
		edgeAdd->draw();
	}
	std::vector<Node *>::iterator it;
	for(it = neurons.begin(); it != neurons.end(); it++)
	{
		Node *node = *it;
		node->draw();
	}
	if(actionState == ActionAdd)
	{
		nodeAdd->draw();
	}
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

void NeuralNetwork::updateAddNeuron(int x, int y)
{
	nodeAdd->setPosition(x, y);
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
	for(ite = edges.begin(); ite != edges.end(); ite++)
	{
		Edge *edge = *ite;
		if(edge->IsIn(p))
			return edge;
	}
	return NULL;
}

bool NeuralNetwork::canConnect(Node *first, Node *second)
{
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
	Edge *edge = new Edge(first, node, edgeAdd->getWeight());
	edges.push_back(edge);
	modified = true;
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


void NeuralNetwork::deleteNode(Node *node)
{
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

void NeuralNetwork::deleteEdge(Edge* edge)
{
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

void NeuralNetwork::clear()
{
	selectNode(NULL);
	selectEdge(NULL);
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
	bDown = false;
	bMoving = false;
	actionState = ActionNone;
}

void NeuralNetwork::notifyDataChanged()
{
	modified = true;
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
	file.write(RN_SIGN, 3); // sign
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
		else
			intVal = 0;
		writeInt32(file, intVal); // activation function
		writeInt32(file, node->getPosition().x); // x position
		writeInt32(file, node->getPosition().y); // y position
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
	if(strcmp((const char*)ver, RN_SIGN) != 0)
	{
		if(!isprint(ver[0]) || !isprint(ver[1]) || !isprint(ver[2]))
			throw std::runtime_error("Not a neural network file.");
		throw std::runtime_error(std::string("Version ") + (const char*)ver +
			" not supported, required " RN_SIGN ".");
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
		if(node.type == Node::TypeNeuron &&
			(node.function < 0 || node.function >= NeuronNode::FunctionCount))
			throw std::runtime_error("Invalid activation function.");
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
			NeuronNode *neuron = new NeuronNode(neuronImg, funcImgs);
			neuron->setFunctionType(nodes[n].function);
			node = neuron;
		}
		else if(nodes[n].type == Node::TypeInput)
			node = new InputNode(inputImg);
		else
			node = new OutputNode(outputImg);
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
}
