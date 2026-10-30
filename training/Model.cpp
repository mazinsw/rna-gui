#include "Model.h"
#include <algorithm>
#include <cmath>
#include <ctype.h>
#include <fstream>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#include <string>

/*
 * Formato 2.0 do arquivo, little-endian com tamanhos fixos para ser o mesmo em
 * qualquer plataforma:
 * sign[3] inputs:u32 side:u32 layerCount:u32
 * layerCount x (size:u32 function:i32 beta:f32
 *               weights:f32[inputCount x size] bias:f32[size])
 * Os pesos ficam na ordem de Layer::weights, os de cada entrada juntos.
 *
 * Os formatos 1.1 e 1.2 são os da rede em grafo, a 1.1 gravada pela versão
 * Windows 32 bits:
 * sign[3] nodeCount:u32 edgeCount:u32
 * nodeCount x (type:i32 function:i32 x:i32 y:i32 [bias:f64 na 1.2])
 * edgeCount x (first:i32 second:i32 weight:f64)
 * O tipo é 1 no neurônio, 2 na entrada e 3 na saída; na 1.2 a função da
 * entrada é 1 na entrada de bias, que vale sempre 1.
 */
#define SIGN "2.0"
#define GRAPH_SIGN "1.1"
#define GRAPH_SIGN_BIAS "1.2"
// tipos de nó da rede em grafo
#define GRAPH_NEURON 1
#define GRAPH_INPUT 2
#define GRAPH_OUTPUT 3
// a partir de 4 x 4 entradas em quadrado a rede antiga recebe imagens
#define GRAPH_MIN_IMAGE_SIDE 4

/**
 * Caminho em UTF-8, para as mensagens
 */
static std::string utf8(const std::filesystem::path& path)
{
	std::u8string str = path.u8string();
	return std::string(str.begin(), str.end());
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

/**
 * Lê count floats de uma vez
 */
static void readFloats(std::ifstream& file, std::vector<float>& values, size_t count)
{
	std::vector<unsigned char> bytes(count * 4);
	readBytes(file, bytes.data(), bytes.size());
	values.resize(count);
	for(size_t i = 0; i < count; i++)
	{
		uint32_t bits = 0;
		for(int b = 0; b < 4; b++)
			bits |= (uint32_t)bytes[i * 4 + b] << (b * 8);
		memcpy(&values[i], &bits, sizeof(float));
	}
}

static void writeUInt32(std::ofstream& file, uint32_t value)
{
	unsigned char bytes[4];
	for(int i = 0; i < 4; i++)
		bytes[i] = (unsigned char)(value >> (i * 8));
	file.write((const char*)bytes, sizeof(bytes));
}

static void writeFloats(std::ofstream& file, const float* values, size_t count)
{
	std::vector<unsigned char> bytes(count * 4);
	for(size_t i = 0; i < count; i++)
	{
		uint32_t bits;
		memcpy(&bits, &values[i], sizeof(bits));
		for(int b = 0; b < 4; b++)
			bytes[i * 4 + b] = (unsigned char)(bits >> (b * 8));
	}
	file.write((const char*)bytes.data(), bytes.size());
}

Model::Model()
{
	inputs = 0;
	side = 0;
}

int Model::inputCount(int layer) const
{
	return layer == 0 ? inputs : layers[layer - 1].size;
}

int Model::outputs() const
{
	return layers.empty() ? 0 : layers.back().size;
}

Model::Layer Model::layer(int inputs, int size, int function)
{
	return Layer{ size, function, 1, std::vector<float>((size_t)inputs * size, 0),
		std::vector<float>(size, 0) };
}

float Model::activate(int function, float beta, float x)
{
	switch(function)
	{
	case Degrau:
		return x >= 0 ? 1 : 0;
	case Bipolar:
		return x >= 0 ? 1 : -1;
	case Logistic:
		return 1 / (1 + expf(-beta * x));
	case Hiperbolic:
		// (1 - e^(-beta x)) / (1 + e^(-beta x)) sem estourar: e^(-beta x) vira
		// infinito para x muito negativo e a divisão daria nan
		return tanhf(beta * x / 2);
	case Relu:
		return x > 0 ? x : 0;
	default:
		return x;
	}
}

void Model::activate(const Layer& layer, float* values, int count)
{
	size_t total = (size_t)count * layer.size;
	if(layer.function != Softmax)
	{
		if(layer.function == Linear)
			return;
		for(size_t i = 0; i < total; i++)
			values[i] = activate(layer.function, layer.beta, values[i]);
		return;
	}
	// e^x de cada um dividido pela soma, com o maior x subtraído para e^x não
	// estourar
	for(size_t row = 0; row < total; row += layer.size)
	{
		float *value = values + row;
		float highest = -INFINITY, sum = 0;
		for(int j = 0; j < layer.size; j++)
			highest = std::max(highest, value[j]);
		for(int j = 0; j < layer.size; j++)
		{
			value[j] = expf(value[j] - highest);
			sum += value[j];
		}
		for(int j = 0; j < layer.size; j++)
			value[j] /= sum;
	}
}

float Model::derivative(int function, float beta, float y)
{
	if(function == Logistic)
		return beta * y * (1 - y);
	if(function == Hiperbolic)
		return beta * (1 - y * y) / 2;
	if(function == Relu)
		return y > 0 ? 1 : 0;
	return 1;
}

void Model::forward(const float* input, int count, std::vector<std::vector<float> >& values) const
{
	values.resize(layers.size());
	const float* in = input;
	for(size_t l = 0; l < layers.size(); l++)
	{
		const Layer& layer = layers[l];
		int size = layer.size, k = inputCount((int)l);
		values[l].resize((size_t)count * size);
		float* out = values[l].data();
		for(int s = 0; s < count; s++)
		{
			float* sum = out + (size_t)s * size;
			const float* x = in + (size_t)s * k;
			std::copy(layer.bias.begin(), layer.bias.end(), sum);
			for(int i = 0; i < k; i++)
			{
				// as entradas em 0, a maior parte dos pixels de uma imagem, não
				// somam nada
				float value = x[i];
				if(value == 0)
					continue;
				const float* weight = layer.weights.data() + (size_t)i * size;
				for(int j = 0; j < size; j++)
					sum[j] += value * weight[j];
			}
		}
		activate(layer, out, count);
		in = out;
	}
}

std::vector<float> Model::run(const std::vector<float>& input) const
{
	if(layers.empty())
		return std::vector<float>();
	std::vector<std::vector<float> > values;
	forward(input.data(), 1, values);
	return values.back();
}

float Model::randomWeight(std::mt19937& random, int inputs, int size)
{
	// a soma de muitas entradas com pesos grandes saturaria a função
	double range = std::min(1.0, sqrt(6.0 / std::max(1, inputs + size)));
	int steps = std::max(1, (int)(range * 100));
	std::uniform_int_distribution<int> distribution(-steps, steps);
	return distribution(random) / 100.0f;
}

void Model::randomize(std::mt19937& random)
{
	for(size_t l = 0; l < layers.size(); l++)
	{
		Layer& layer = layers[l];
		int k = inputCount((int)l);
		for(float& weight : layer.weights)
			weight = randomWeight(random, k, layer.size);
		for(float& bias : layer.bias)
			bias = randomWeight(random, k, layer.size);
	}
}

bool Model::isFinite() const
{
	for(const Layer& layer : layers)
	{
		for(float weight : layer.weights)
		{
			if(!std::isfinite(weight))
				return false;
		}
		for(float bias : layer.bias)
		{
			if(!std::isfinite(bias))
				return false;
		}
	}
	return true;
}

void Model::resizeInputs(int index, int count, std::mt19937& random)
{
	Layer& layer = layers[index];
	size_t old = layer.weights.size();
	layer.weights.resize((size_t)count * layer.size);
	for(size_t i = old; i < layer.weights.size(); i++)
		layer.weights[i] = randomWeight(random, count, layer.size);
}

void Model::resize(int index, int size, std::mt19937& random)
{
	Layer& layer = layers[index];
	int k = inputCount(index);
	std::vector<float> weights((size_t)k * size);
	for(int i = 0; i < k; i++)
	{
		for(int j = 0; j < size; j++)
			weights[(size_t)i * size + j] = j < layer.size ? layer.weights[(size_t)i * layer.size + j] :
				randomWeight(random, k, size);
	}
	layer.weights = weights;
	for(int j = layer.size; j < size; j++)
		layer.bias.push_back(randomWeight(random, k, size));
	layer.bias.resize(size);
	layer.size = size;
	if(index + 1 < (int)layers.size())
		resizeInputs(index + 1, size, random);
}

void Model::setInputs(int count, int side, std::mt19937& random)
{
	inputs = count;
	this->side = side;
	if(!layers.empty())
		resizeInputs(0, count, random);
}

void Model::insertLayer(int index, int size, int function, std::mt19937& random)
{
	layers.insert(layers.begin() + index, layer(inputCount(index), size, function));
	// a seguinte recebia de outra camada, os pesos dela não valem mais
	for(int l = index; l <= index + 1 && l < (int)layers.size(); l++)
	{
		Layer& layer = layers[l];
		layer.weights.assign((size_t)inputCount(l) * layer.size, 0);
		for(float& weight : layer.weights)
			weight = randomWeight(random, inputCount(l), layer.size);
		if(l == index)
		{
			for(float& bias : layer.bias)
				bias = randomWeight(random, inputCount(l), layer.size);
		}
	}
}

void Model::removeLayer(int index, std::mt19937& random)
{
	layers.erase(layers.begin() + index);
	if(index >= (int)layers.size())
		return;
	Layer& layer = layers[index];
	layer.weights.assign((size_t)inputCount(index) * layer.size, 0);
	for(float& weight : layer.weights)
		weight = randomWeight(random, inputCount(index), layer.size);
}

int Model::imageSide(int count)
{
	int side = (int)lround(sqrt(count));
	return side >= 2 && side * side == count ? side : 0;
}

/**
 * Lê a rede no formato 2.0, depois da assinatura
 */
static Model read(std::ifstream& file, uintmax_t fileSize)
{
	Model model;
	model.inputs = (int)readUInt32(file);
	model.side = (int)readUInt32(file);
	uint32_t count = readUInt32(file);
	if(model.inputs < 0 || model.side < 0 || (model.side > 0 &&
		(int64_t)model.side * model.side != model.inputs))
		throw std::runtime_error("Invalid number of inputs.");
	for(uint32_t l = 0; l < count; l++)
	{
		int size = (int)readUInt32(file);
		int function = readInt32(file);
		std::vector<float> beta;
		readFloats(file, beta, 1);
		int k = model.inputCount((int)l);
		// um tamanho além do arquivo daria uma alocação enorme antes do fim dele
		if(size < 1 || (uintmax_t)k * size * 4 > fileSize)
			throw std::runtime_error("Invalid layer size.");
		if(function < 0 || function >= Model::FunctionCount)
			throw std::runtime_error("Invalid activation function.");
		Model::Layer layer = Model::layer(0, size, function);
		layer.beta = beta[0];
		readFloats(file, layer.weights, (size_t)k * size);
		readFloats(file, layer.bias, size);
		model.layers.push_back(layer);
	}
	return model;
}

/**
 * Lê a rede em grafo dos formatos 1.1 e 1.2, depois da assinatura, e a
 * converte em camadas
 */
static Model readGraph(std::ifstream& file, bool hasBias)
{
	struct Node
	{
		int type;
		int function;
		double bias;
		bool biasInput;
		// camada do neurônio, de 1 em diante, 0 ainda não calculada e -1
		// calculando
		int layer;
		// posição na camada ou nas entradas, -1 na entrada de bias
		int index;
	};
	struct Edge
	{
		int source, target;
		double weight;
	};
	uint32_t nodeCount = readUInt32(file); // neuron count
	uint32_t edgeCount = readUInt32(file); // edge count
	std::vector<Node> nodes;
	for(uint32_t i = 0; i < nodeCount; i++)
	{
		Node node = { readInt32(file), readInt32(file), 0, false, 0, -1 };
		readInt32(file); // x position
		readInt32(file); // y position
		node.bias = hasBias ? readDouble(file) : 0;
		if(node.type == GRAPH_NEURON && (node.function < 0 || node.function >= Model::FunctionCount))
			throw std::runtime_error("Invalid activation function.");
		// a 1.1 não marca entradas de bias
		if(node.type == GRAPH_INPUT && hasBias)
		{
			if(node.function != 0 && node.function != 1)
				throw std::runtime_error("Invalid input kind.");
			node.biasInput = node.function == 1;
		}
		else if(node.type != GRAPH_NEURON && node.type != GRAPH_INPUT)
			node.type = GRAPH_OUTPUT;
		nodes.push_back(node);
	}
	// as ligações que a rede em grafo ignorava: de um nó a ele mesmo,
	// repetidas e entre entradas e saídas
	std::vector<Edge> edges;
	std::set<std::pair<int, int> > linked;
	for(uint32_t i = 0; i < edgeCount; i++)
	{
		int first = readInt32(file); // first node id
		int second = readInt32(file); // second node id
		double weight = readDouble(file); // connection weight
		if(first < 1 || first > (int)nodeCount || second < 1 || second > (int)nodeCount)
			throw std::runtime_error("Invalid node id.");
		int source = first - 1, target = second - 1;
		int from = nodes[source].type, to = nodes[target].type;
		// a ligação que chega numa entrada também não mudava nada
		if(source == target || !linked.insert(std::make_pair(source, target)).second ||
			(from != GRAPH_NEURON && to != GRAPH_NEURON) || to == GRAPH_INPUT)
			continue;
		if(from == GRAPH_OUTPUT)
			throw std::runtime_error("Not a layered network: an output feeds a neuron.");
		edges.push_back(Edge{ source, target, weight });
	}
	// camada do neurônio: uma depois da mais alta dos neurônios que o
	// alimentam
	std::vector<std::vector<int> > sources(nodes.size());
	for(const Edge& edge : edges)
	{
		if(nodes[edge.source].type == GRAPH_NEURON)
			sources[edge.target].push_back(edge.source);
	}
	int layerCount = 0;
	for(size_t n = 0; n < nodes.size(); n++)
	{
		if(nodes[n].type != GRAPH_NEURON || nodes[n].layer > 0)
			continue;
		// em profundidade sem recursão: a pilha guarda o nó e a próxima fonte
		std::vector<std::pair<int, size_t> > stack(1, std::make_pair((int)n, (size_t)0));
		nodes[n].layer = -1;
		while(!stack.empty())
		{
			int node = stack.back().first;
			size_t& next = stack.back().second;
			if(next < sources[node].size())
			{
				int source = sources[node][next++];
				if(nodes[source].layer < 0)
					throw std::runtime_error("Not a layered network: it has a cycle.");
				if(nodes[source].layer == 0)
				{
					nodes[source].layer = -1;
					stack.push_back(std::make_pair(source, (size_t)0));
				}
				continue;
			}
			int layer = 1;
			for(int source : sources[node])
				layer = std::max(layer, nodes[source].layer + 1);
			nodes[node].layer = layer;
			layerCount = std::max(layerCount, layer);
			stack.pop_back();
		}
	}
	Model model;
	std::vector<int> layerSizes(layerCount + 1, 0);
	std::vector<int> outputs;
	for(size_t n = 0; n < nodes.size(); n++)
	{
		Node& node = nodes[n];
		if(node.type == GRAPH_INPUT && !node.biasInput)
			node.index = model.inputs++;
		else if(node.type == GRAPH_NEURON)
			node.index = layerSizes[node.layer]++;
		else if(node.type == GRAPH_OUTPUT)
			outputs.push_back((int)n);
	}
	if(model.inputs >= GRAPH_MIN_IMAGE_SIDE * GRAPH_MIN_IMAGE_SIDE)
		model.side = Model::imageSide(model.inputs);
	for(int l = 1; l <= layerCount; l++)
	{
		int function = -1;
		for(const Node& node : nodes)
		{
			if(node.type != GRAPH_NEURON || node.layer != l)
				continue;
			if(function >= 0 && node.function != function)
				throw std::runtime_error("Not a layered network: the neurons of layer " +
					std::to_string(l) + " have different activation functions.");
			function = node.function;
		}
		model.layers.push_back(Model::layer(model.inputCount(l - 1), layerSizes[l], function));
	}
	for(const Node& node : nodes)
	{
		if(node.type == GRAPH_NEURON)
			model.layers[node.layer - 1].bias[node.index] = (float)node.bias;
	}
	// cada saída repassa o valor de um neurônio da última camada, com peso 1 e
	// todos com saída, ou uma camada linear soma o que chega nelas
	std::vector<int> fed(nodes.size(), 0);
	bool direct = layerCount > 0 && (int)outputs.size() == layerSizes[layerCount];
	for(const Edge& edge : edges)
	{
		const Node& source = nodes[edge.source];
		if(nodes[edge.target].type != GRAPH_OUTPUT)
			continue;
		if(source.layer != layerCount)
			throw std::runtime_error("Not a layered network: an output is fed by a neuron "
				"outside the last layer.");
		direct = direct && edge.weight == 1 && fed[edge.source] == 0;
		fed[edge.source]++;
		fed[edge.target]++;
	}
	for(int output : outputs)
		direct = direct && fed[output] == 1;
	if(!outputs.empty() && !direct)
		model.layers.push_back(Model::layer(model.inputCount(layerCount), (int)outputs.size(),
			Model::Linear));
	// a camada de saída na ordem das saídas
	std::vector<int> order(outputs.size());
	for(size_t o = 0; o < outputs.size(); o++)
		nodes[outputs[o]].index = (int)o;
	for(const Edge& edge : edges)
	{
		const Node& source = nodes[edge.source];
		const Node& target = nodes[edge.target];
		if(source.biasInput)
			model.layers[target.layer - 1].bias[target.index] += (float)edge.weight;
		else if(source.type == GRAPH_INPUT)
		{
			if(target.layer != 1)
				throw std::runtime_error("Not a layered network: an input feeds a neuron outside "
					"the first layer.");
			Model::Layer& layer = model.layers[0];
			layer.weights[(size_t)source.index * layer.size + target.index] = (float)edge.weight;
		}
		else if(target.type == GRAPH_NEURON)
		{
			if(target.layer != source.layer + 1)
				throw std::runtime_error("Not a layered network: a connection skips a layer.");
			Model::Layer& layer = model.layers[target.layer - 1];
			layer.weights[(size_t)source.index * layer.size + target.index] = (float)edge.weight;
		}
		else if(direct)
			order[target.index] = source.index;
		else
		{
			Model::Layer& layer = model.layers.back();
			layer.weights[(size_t)source.index * layer.size + target.index] = (float)edge.weight;
		}
	}
	if(direct)
	{
		// reordena os neurônios da última camada e as linhas dos pesos deles
		Model::Layer& last = model.layers.back();
		Model::Layer sorted = last;
		int k = model.inputCount((int)model.layers.size() - 1);
		for(size_t o = 0; o < order.size(); o++)
		{
			sorted.bias[o] = last.bias[order[o]];
			for(int i = 0; i < k; i++)
				sorted.weights[(size_t)i * last.size + o] = last.weights[(size_t)i * last.size + order[o]];
		}
		last = sorted;
	}
	return model;
}

Model Model::load(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	if(!file)
		throw std::runtime_error("Unable to open " + utf8(path) + ".");
	std::error_code error;
	uintmax_t size = std::filesystem::file_size(path, error);
	unsigned char sign[4] = { 0, 0, 0, 0 };
	readBytes(file, sign, 3);
	if(strcmp((const char*)sign, SIGN) == 0)
		return read(file, error ? 0 : size);
	if(strcmp((const char*)sign, GRAPH_SIGN) == 0 || strcmp((const char*)sign, GRAPH_SIGN_BIAS) == 0)
		return readGraph(file, strcmp((const char*)sign, GRAPH_SIGN_BIAS) == 0);
	if(!isprint(sign[0]) || !isprint(sign[1]) || !isprint(sign[2]))
		throw std::runtime_error("Not a neural network file.");
	throw std::runtime_error(std::string("Version ") + (const char*)sign +
		" not supported, required " SIGN ", " GRAPH_SIGN " or " GRAPH_SIGN_BIAS ".");
}

void Model::save(const std::filesystem::path& path) const
{
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if(!file)
		throw std::runtime_error("Unable to create " + utf8(path) + ".");
	file.write(SIGN, 3);
	writeUInt32(file, (uint32_t)inputs);
	writeUInt32(file, (uint32_t)side);
	writeUInt32(file, (uint32_t)layers.size());
	for(const Layer& layer : layers)
	{
		writeUInt32(file, (uint32_t)layer.size);
		writeUInt32(file, (uint32_t)layer.function);
		writeFloats(file, &layer.beta, 1);
		writeFloats(file, layer.weights.data(), layer.weights.size());
		writeFloats(file, layer.bias.data(), layer.bias.size());
	}
	file.close();
	if(!file)
		throw std::runtime_error("Unable to write " + utf8(path) + ".");
}
