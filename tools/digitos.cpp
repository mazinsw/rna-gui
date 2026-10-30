/**
 * Gera o exemplo de reconhecimento de dígitos a partir das imagens de uma
 * pasta, cada uma com o dígito no começo do nome (3_7.png):
 *
 * uso: digitos <pasta> <rede.bin> [lado] [ocultos]
 *
 * Grava a rede, lado x lado entradas (8 x 8 pixels sem o lado) -> ocultos
 * neurônios ReLU (10 sem o número) -> 10 neurônios softmax -> 10 saídas, com
 * pesos sorteados, e ao lado as amostras (rede.amostras.txt), uma por imagem,
 * reduzidas por pixels::fromFile como as imagens arrastadas para a interface.
 */
#include "../gui/Pixels.h"
#include "../gui/Platform.h"
#include "../training/Network.h"
#include "../training/Training.h"
#include <raylib.h>
#include <algorithm>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// classes, os dígitos
static const int Classes = 10;

int main(int argc, char *argv[])
{
	// lado da imagem e neurônios da camada oculta
	int side = argc > 3 ? atoi(argv[3]) : 8;
	int hidden = argc > 4 ? atoi(argv[4]) : 10;
	if(argc < 3 || argc > 5 || side < 2 || hidden < 1)
	{
		fprintf(stderr, "usage: %s <folder> <network.bin> [side] [hidden]\n", argv[0]);
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
		// entradas em quadrado, como os pixels, à esquerda das camadas de nós a
		// 120 pixels um do outro, todos centralizados na altura da maior camada;
		// os centros ficam nos pontos da grade
		const int top = 70, spacing = 120, pixel = 40;
		const int middle = top + spacing * (std::max(hidden, Classes) - 1) / 2;
		const int hiddenTop = middle - spacing * (hidden - 1) / 2;
		const int classesTop = middle - spacing * (Classes - 1) / 2;
		const int left = 250, gridTop = middle - pixel * (side - 1) / 2;
		const int hiddenX = left + pixel * (side - 1) + 240;
		const int softmaxX = hiddenX + 200, outputX = softmaxX + 160;
		Network network;
		auto add = [&](int type, int function, int x, int y)
		{
			Network::Node node = Network::node(type, function);
			node.x = x;
			node.y = y;
			network.nodes.push_back(node);
		};
		for(int i = 0; i < side * side; i++)
			add(Network::Input, Network::Linear, left + pixel * (i % side),
				gridTop + pixel * (i / side));
		int hiddenStart = (int)network.nodes.size();
		for(int i = 0; i < hidden; i++)
			add(Network::Neuron, Network::Relu, hiddenX, hiddenTop + spacing * i);
		int softmaxStart = (int)network.nodes.size();
		for(int i = 0; i < Classes; i++)
			add(Network::Neuron, Network::Softmax, softmaxX, classesTop + spacing * i);
		int outputStart = (int)network.nodes.size();
		for(int i = 0; i < Classes; i++)
			add(Network::Output, Network::Linear, outputX, classesTop + spacing * i);
		for(Network::Node& node : network.nodes)
		{
			if(node.type == Network::Neuron)
				node.bias = randomWeight();
		}
		for(int h = 0; h < hidden; h++)
		{
			for(int i = 0; i < side * side; i++)
				network.edges.push_back({ i, hiddenStart + h, randomWeight(), false });
		}
		for(int c = 0; c < Classes; c++)
		{
			for(int h = 0; h < hidden; h++)
				network.edges.push_back({ hiddenStart + h, softmaxStart + c, randomWeight(), false });
		}
		// a saída ligada a um nó só repassa o valor dele
		for(int c = 0; c < Classes; c++)
			network.edges.push_back({ softmaxStart + c, outputStart + c, 1, false });
		std::vector<Training::Sample> samples;
		for(const std::filesystem::path& image : images)
		{
			std::string name = pathToUtf8(image);
			Training::Sample sample;
			sample.inputs = pixels::fromFile(name, side);
			sample.targets.assign(Classes, 0);
			sample.targets[pixels::labelOf(name)] = 1;
			samples.push_back(sample);
		}
		Training training;
		training.setSamples(samples);
		network.save(pathFromUtf8(argv[2]));
		training.saveSamples(pathFromUtf8(argv[2]));
		printf("%s: %d nodes, %d edges, %d samples\n", argv[2], (int)network.nodes.size(),
			(int)network.edges.size(), (int)samples.size());
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "Error: %s\n", e.what());
		return 1;
	}
	return 0;
}
