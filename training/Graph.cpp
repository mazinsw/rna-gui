#include "Graph.h"
#include <algorithm>
#include <cmath>
#include <ctype.h>
#include <fstream>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#define RN_SIGN "1.1"
#define RN_SIGN_BIAS "1.2"

/*
 * Formato do arquivo, little-endian com tamanhos fixos para ser o mesmo em
 * qualquer plataforma. A versão 1.1 é a gravada pela versão Windows 32 bits:
 * sign[3] neuronCount:u32 edgeCount:u32
 * neuronCount x (type:i32 function:i32 x:i32 y:i32)
 * edgeCount x (first:i32 second:i32 weight:f64)
 * A versão 1.2 acrescenta bias:f64 a cada nó e usa function da entrada para
 * marcar a entrada de bias (1). É gravada só quando a rede usa algum bias ou
 * uma função que a versão Windows não tem (ReLU e softmax), as demais
 * continuam em 1.1.
 */

/**
 * Caminho em UTF-8, para as mensagens
 */
static std::string utf8(const std::filesystem::path& path)
{
	std::u8string str = path.u8string();
	return std::string(str.begin(), str.end());
}

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

Graph::Graph()
{
	cycle = false;
}

Graph::Node Graph::node(int type, int function)
{
	return Node{ type, function, 1, 0, false, 0, 0, 0, 0, false };
}

bool Graph::canConnect(int source, int target)
{
	if(source == Input && (target == Input || target == Output))
		return false;
	if(source == Output && (target == Output || target == Input))
		return false;
	return true;
}

double Graph::activate(int function, double beta, double x)
{
	switch(function)
	{
	case Degrau:
		return x >= 0 ? 1 : 0;
	case Bipolar:
		return x >= 0 ? 1 : -1;
	case Logistic:
		return 1.0 / (1.0 + pow(M_E, - beta * x));
	case Hiperbolic:
		// (1 - e^(-beta x)) / (1 + e^(-beta x)) sem estourar: e^(-beta x) vira
		// infinito para x muito negativo e a divisão daria nan
		return tanh(beta * x / 2);
	case Relu:
		return x > 0 ? x : 0;
	default:
		return x;
	}
}

double Graph::derivative(int function, double beta, double y)
{
	if(function == Logistic)
		return beta * y * (1 - y);
	if(function == Hiperbolic)
		return beta * (1 - y * y) / 2;
	if(function == Relu)
		return y > 0 ? 1 : 0;
	return 1;
}

void Graph::sort()
{
	order.clear();
	inputs.clear();
	outputs.clear();
	neurons.clear();
	softmax.clear();
	cycle = false;
	// cada nó guarda o fim das suas conexões e depois o começo
	incomingStart.assign(nodes.size() + 1, 0);
	outgoingStart.assign(nodes.size() + 1, 0);
	for(const Edge& edge : edges)
	{
		incomingStart[edge.target + 1]++;
		outgoingStart[edge.source + 1]++;
	}
	for(size_t i = 0; i < nodes.size(); i++)
	{
		incomingStart[i + 1] += incomingStart[i];
		outgoingStart[i + 1] += outgoingStart[i];
	}
	incomingEdges.assign(edges.size(), 0);
	outgoingEdges.assign(edges.size(), 0);
	std::vector<int> incomingNext(incomingStart.begin(), incomingStart.end() - 1);
	std::vector<int> outgoingNext(outgoingStart.begin(), outgoingStart.end() - 1);
	for(size_t i = 0; i < edges.size(); i++)
	{
		incomingEdges[incomingNext[edges[i].target]++] = (int)i;
		outgoingEdges[outgoingNext[edges[i].source]++] = (int)i;
	}
	for(size_t i = 0; i < nodes.size(); i++)
	{
		if(nodes[i].type == Input)
			inputs.push_back((int)i);
		else if(nodes[i].type == Output)
			outputs.push_back((int)i);
		else
		{
			neurons.push_back((int)i);
			if(isSoftmax((int)i))
				softmax.push_back((int)i);
		}
	}
	std::vector<int> state(nodes.size(), 0);
	for(size_t i = 0; i < nodes.size(); i++)
		visit((int)i, state);
}

bool Graph::isSoftmax(int node)
{
	return nodes[node].type == Neuron && nodes[node].function == Softmax;
}

void Graph::visit(int node, std::vector<int>& state)
{
	// 1: visitando, 2: visitado
	if(state[node] == 2)
		return;
	if(state[node] == 1)
	{
		cycle = true;
		return;
	}
	std::vector<int> group(1, node);
	if(isSoftmax(node))
		group = softmax;
	for(int member : group)
		state[member] = 1;
	for(int member : group)
	{
		for(int i = incomingStart[member]; i < incomingStart[member + 1]; i++)
			visit(edges[incomingEdges[i]].source, state);
	}
	for(int member : group)
	{
		state[member] = 2;
		order.push_back(member);
	}
}

double Graph::sum(int node)
{
	double sum = 0;
	for(int i = incomingStart[node]; i < incomingStart[node + 1]; i++)
	{
		const Edge& edge = edges[incomingEdges[i]];
		sum += edge.weight * nodes[edge.source].value;
	}
	return sum;
}

void Graph::propagate()
{
	for(int index : order)
	{
		Node& node = nodes[index];
		if(node.type == Input)
			continue;
		if(!isSoftmax(index))
		{
			double value = sum(index);
			if(node.type == Neuron)
				value = activate(node.function, node.beta, value + node.bias);
			node.value = value;
			continue;
		}
		// os neurônios softmax estão juntos na ordem, o primeiro calcula todos:
		// e^x de cada um dividido pela soma, com o maior x subtraído para e^x
		// não estourar
		if(index != softmax[0])
			continue;
		std::vector<double> values;
		double highest = -INFINITY;
		for(int member : softmax)
		{
			values.push_back(sum(member) + nodes[member].bias);
			highest = std::max(highest, values.back());
		}
		double total = 0;
		for(double& value : values)
		{
			value = exp(value - highest);
			total += value;
		}
		for(size_t i = 0; i < softmax.size(); i++)
			nodes[softmax[i]].value = values[i] / total;
	}
}

bool Graph::execute()
{
	sort();
	propagate();
	return !cycle;
}

bool Graph::hasCycle()
{
	return cycle;
}

std::vector<bool> Graph::isTrainable()
{
	std::vector<int> count(nodes.size(), 0);
	for(const Edge& edge : edges)
		count[edge.target]++;
	std::vector<bool> trainable;
	for(const Edge& edge : edges)
	{
		const Node& target = nodes[edge.target];
		trainable.push_back(target.type == Neuron ||
			(target.type == Output && count[edge.target] > 1));
	}
	return trainable;
}

std::vector<int> Graph::readingOrder(const std::vector<std::pair<int, int> >& positions)
{
	std::vector<int> result;
	for(size_t i = 0; i < positions.size(); i++)
		result.push_back((int)i);
	std::stable_sort(result.begin(), result.end(), [&](int a, int b)
	{
		const std::pair<int, int>& p = positions[a];
		const std::pair<int, int>& q = positions[b];
		return p.second != q.second ? p.second < q.second : p.first < q.first;
	});
	return result;
}

int Graph::imageSide(int count)
{
	int side = (int)lround(sqrt(count));
	return side >= 2 && side * side == count ? side : 0;
}

std::vector<int> Graph::readingOrder(int type)
{
	std::vector<int> indexes;
	std::vector<std::pair<int, int> > positions;
	for(size_t i = 0; i < nodes.size(); i++)
	{
		if(nodes[i].type != type || nodes[i].biasInput)
			continue;
		indexes.push_back((int)i);
		positions.push_back(std::make_pair(nodes[i].x, nodes[i].y));
	}
	std::vector<int> result;
	for(int i : readingOrder(positions))
		result.push_back(indexes[i]);
	return result;
}

int Graph::getImageSide()
{
	return imageSide((int)readingOrder(Input).size());
}

bool Graph::showImage(const std::vector<double>& values, int label)
{
	std::vector<int> pixels = readingOrder(Input);
	for(size_t i = 0; i < pixels.size() && i < values.size(); i++)
		nodes[pixels[i]].value = values[i];
	std::vector<int> classes = readingOrder(Output);
	for(size_t i = 0; i < classes.size(); i++)
		nodes[classes[i]].target = (int)i == label ? 1 : 0;
	execute();
	return label >= 0 && label < (int)classes.size();
}

int Graph::getRecognized()
{
	std::vector<int> classes = readingOrder(Output);
	int recognized = -1;
	for(size_t i = 0; i < classes.size(); i++)
	{
		double value = nodes[classes[i]].value;
		if(!std::isfinite(value))
			return -1;
		if(recognized < 0 || value > nodes[classes[recognized]].value)
			recognized = (int)i;
	}
	return recognized;
}

double Graph::getOutputValue(int output)
{
	return nodes[readingOrder(Output)[output]].value;
}

Graph Graph::load(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	unsigned char ver[4];
	if(!file)
		throw std::runtime_error("Unable to open " + utf8(path) + ".");
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
	uint32_t nodeCount = readUInt32(file); // neuron count
	uint32_t edgeCount = readUInt32(file); // edge count
	Graph network;
	for(uint32_t i = 0; i < nodeCount; i++)
	{
		int type = readInt32(file); // type, 1 -> Neuron, 2 -> Input, 3 ->Output
		int function = readInt32(file); // activation function
		Node node = Graph::node(type);
		node.x = readInt32(file); // x position
		node.y = readInt32(file); // y position
		double bias = hasBias ? readDouble(file) : 0; // neuron bias
		if(type == Neuron)
		{
			if(function < 0 || function >= FunctionCount)
				throw std::runtime_error("Invalid activation function.");
			node.function = function;
			node.bias = bias;
		}
		// a 1.1 não marca entradas de bias
		else if(type == Input && hasBias)
		{
			if(function != 0 && function != 1)
				throw std::runtime_error("Invalid input kind.");
			node.biasInput = function == 1;
			node.value = node.biasInput ? 1 : 0;
		}
		else if(type != Input)
			node.type = Output;
		network.nodes.push_back(node);
	}
	std::set<std::pair<int, int> > linked;
	for(uint32_t i = 0; i < edgeCount; i++)
	{
		int first = readInt32(file); // first node id
		int second = readInt32(file); // second node id
		double weight = readDouble(file); // connection weight
		if(first < 1 || first > (int)nodeCount || second < 1 || second > (int)nodeCount)
			throw std::runtime_error("Invalid node id.");
		int source = first - 1, target = second - 1;
		if(source == target || !linked.insert(std::make_pair(source, target)).second ||
			!canConnect(network.nodes[source].type, network.nodes[target].type))
			continue;
		network.edges.push_back(Edge{ source, target, weight, false });
	}
	return network;
}

void Graph::save(const std::filesystem::path& path)
{
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if(!file)
		throw std::runtime_error("Unable to create " + utf8(path) + ".");
	bool hasBias = false;
	for(const Node& node : nodes)
	{
		if((node.type == Neuron && (node.bias != 0 || node.function >= Relu)) ||
			(node.type == Input && node.biasInput))
			hasBias = true;
	}
	file.write(hasBias ? RN_SIGN_BIAS : RN_SIGN, 3); // sign
	writeUInt32(file, (uint32_t)nodes.size()); // neuron count
	writeUInt32(file, (uint32_t)edges.size()); // edge count
	for(const Node& node : nodes)
	{
		writeInt32(file, node.type); // type, 1 -> Neuron, 2 -> Input, 3 ->Output
		int function = 0;
		if(node.type == Neuron)
			function = node.function;
		else if(node.type == Input && node.biasInput)
			function = 1;
		writeInt32(file, function); // activation function, 1 on a bias input
		writeInt32(file, node.x); // x position
		writeInt32(file, node.y); // y position
		if(hasBias)
			writeDouble(file, node.type == Neuron ? node.bias : 0); // neuron bias
	}
	for(const Edge& edge : edges)
	{
		writeInt32(file, edge.source + 1); // first node id
		writeInt32(file, edge.target + 1); // second node id
		writeDouble(file, edge.weight); // connection weight
	}
	file.close();
	if(!file)
		throw std::runtime_error("Unable to write " + utf8(path) + ".");
}
