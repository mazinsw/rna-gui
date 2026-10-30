/**
 * Gera o exemplo de reconhecimento de dígitos a partir das imagens de uma
 * pasta, cada uma com o dígito no começo do nome (3_7.png):
 *
 * uso: digitos <pasta> <rede.bin>
 *
 * Grava a rede, 64 entradas (8 x 8 pixels) -> 10 neurônios ReLU -> 10
 * neurônios softmax -> 10 saídas, com pesos sorteados, e ao lado as amostras
 * (rede.amostras.txt), uma por imagem, reduzidas por pixels::fromFile como as
 * imagens arrastadas para a interface.
 */
#include "../gui/Pixels.h"
#include "../gui/Platform.h"
#include <raylib.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

// lado da imagem, neurônios da camada oculta e classes (dígitos)
static const int Side = 8;
static const int Hidden = 10;
static const int Classes = 10;
// mesmos valores de Node::Type e NeuronNode::FunctionType
enum { TypeNeuron = 1, TypeInput, TypeOutput };
enum { FunctionRelu = 5, FunctionSoftmax = 6 };

struct NodeData
{
	int type, function, x, y;
	double bias;
};

struct EdgeData
{
	// posição dos nós na lista, a partir de 1
	int first, second;
	double weight;
};

static void writeUInt32(std::ofstream& file, uint32_t value)
{
	unsigned char bytes[4];
	for(int i = 0; i < 4; i++)
		bytes[i] = (unsigned char)(value >> (i * 8));
	file.write((const char*)bytes, sizeof(bytes));
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

/**
 * Rede no formato 1.2 da interface, veja NeuralNetwork.cpp
 */
static void saveNetwork(const std::filesystem::path& path, const std::vector<NodeData>& nodes,
	const std::vector<EdgeData>& edges)
{
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if(!file)
		throw std::runtime_error("Unable to create " + pathToUtf8(path) + ".");
	file.write("1.2", 3);
	writeUInt32(file, (uint32_t)nodes.size());
	writeUInt32(file, (uint32_t)edges.size());
	for(const NodeData& node : nodes)
	{
		writeUInt32(file, (uint32_t)node.type);
		writeUInt32(file, (uint32_t)node.function);
		writeUInt32(file, (uint32_t)node.x);
		writeUInt32(file, (uint32_t)node.y);
		writeDouble(file, node.bias);
	}
	for(const EdgeData& edge : edges)
	{
		writeUInt32(file, (uint32_t)edge.first);
		writeUInt32(file, (uint32_t)edge.second);
		writeDouble(file, edge.weight);
	}
	file.close();
	if(!file)
		throw std::runtime_error("Unable to write " + pathToUtf8(path) + ".");
}

/**
 * Número com ponto decimal e só os dígitos necessários para ser lido igual
 */
static std::string formatValue(double value)
{
	char str[32];
	snprintf(str, sizeof(str), "%.15g", value);
	if(strtod(str, NULL) != value)
		snprintf(str, sizeof(str), "%.17g", value);
	return str;
}

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		fprintf(stderr, "usage: %s <folder> <network.bin>\n", argv[0]);
		return 1;
	}
	SetTraceLogLevel(LOG_WARNING);
	try
	{
		std::vector<std::filesystem::path> images;
		for(const auto& entry : std::filesystem::directory_iterator(pathFromUtf8(argv[1])))
		{
			std::string name = pathToUtf8(entry.path());
			if(pixels::isImage(name) && pixels::labelOf(name) >= 0)
				images.push_back(entry.path());
		}
		std::sort(images.begin(), images.end());
		if(images.empty())
			throw std::runtime_error(std::string("No image named after its digit in ") +
				argv[1] + ".");
		// pesos entre -1 e 1 com 2 casas, como os da interface, sempre os mesmos
		std::mt19937 random(1);
		std::uniform_int_distribution<int> distribution(-100, 100);
		auto randomWeight = [&]() { return distribution(random) / 100.0; };
		// entradas em quadrado, como os pixels, à esquerda das camadas de 10
		// nós a 120 pixels um do outro; os centros ficam nos pontos da grade
		const int top = 70, spacing = 120, pixel = 40;
		const int middle = top + spacing * (Classes - 1) / 2;
		const int left = 250, gridTop = middle - pixel * (Side - 1) / 2;
		const int hiddenX = left + pixel * (Side - 1) + 240;
		const int softmaxX = hiddenX + 200, outputX = softmaxX + 160;
		std::vector<NodeData> nodes;
		std::vector<EdgeData> edges;
		for(int i = 0; i < Side * Side; i++)
			nodes.push_back({ TypeInput, 0, left + pixel * (i % Side), gridTop + pixel * (i / Side),
				0 });
		int hiddenStart = (int)nodes.size() + 1;
		for(int i = 0; i < Hidden; i++)
			nodes.push_back({ TypeNeuron, FunctionRelu, hiddenX, top + spacing * i, 0 });
		int softmaxStart = (int)nodes.size() + 1;
		for(int i = 0; i < Classes; i++)
			nodes.push_back({ TypeNeuron, FunctionSoftmax, softmaxX, top + spacing * i, 0 });
		int outputStart = (int)nodes.size() + 1;
		for(int i = 0; i < Classes; i++)
			nodes.push_back({ TypeOutput, 0, outputX, top + spacing * i, 0 });
		for(NodeData& node : nodes)
		{
			if(node.type == TypeNeuron)
				node.bias = randomWeight();
		}
		for(int h = 0; h < Hidden; h++)
		{
			for(int i = 0; i < Side * Side; i++)
				edges.push_back({ 1 + i, hiddenStart + h, randomWeight() });
		}
		for(int c = 0; c < Classes; c++)
		{
			for(int h = 0; h < Hidden; h++)
				edges.push_back({ hiddenStart + h, softmaxStart + c, randomWeight() });
		}
		// a saída ligada a um nó só repassa o valor dele
		for(int c = 0; c < Classes; c++)
			edges.push_back({ softmaxStart + c, outputStart + c, 1 });
		std::filesystem::path network = pathFromUtf8(argv[2]);
		saveNetwork(network, nodes, edges);
		std::filesystem::path samplesPath = network;
		samplesPath.replace_extension(".amostras.txt");
		std::ofstream samples(samplesPath, std::ios::binary | std::ios::trunc);
		if(!samples)
			throw std::runtime_error("Unable to create " + pathToUtf8(samplesPath) + ".");
		samples << "amostras " << images.size() << "\n";
		for(const std::filesystem::path& image : images)
		{
			std::string name = pathToUtf8(image);
			std::vector<double> values = pixels::fromFile(name, Side);
			for(size_t i = 0; i < values.size(); i++)
				samples << (i > 0 ? " " : "") << formatValue(values[i]);
			samples << "\nsaidas";
			for(int c = 0; c < Classes; c++)
				samples << (c == pixels::labelOf(name) ? " 1" : " 0");
			samples << "\n";
		}
		samples.close();
		if(!samples)
			throw std::runtime_error("Unable to write " + pathToUtf8(samplesPath) + ".");
		printf("%s: %d nodes, %d edges, %d samples\n", argv[2], (int)nodes.size(),
			(int)edges.size(), (int)images.size());
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "Error: %s\n", e.what());
		return 1;
	}
	return 0;
}
