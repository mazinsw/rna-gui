/**
 * Gera o exemplo de reconhecimento de dígitos a partir das imagens de uma
 * pasta, cada uma com o dígito no começo do nome (3_7.png):
 *
 * uso: digitos <pasta> <rede.bin> [lado] [ocultos]
 *
 * Grava a rede, lado x lado entradas (8 x 8 pixels sem o lado) -> ocultos
 * neurônios ReLU (10 sem o número) -> 10 saídas softmax, com pesos sorteados,
 * e ao lado as amostras (rede.amostras.txt), uma por imagem, reduzidas por
 * pixels::fromFile como as imagens arrastadas para a interface.
 */
#include "../gui/Pixels.h"
#include "../gui/Platform.h"
#include "../training/Network.h"
#include <raylib.h>
#include <random>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

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
		Dataset samples = pixels::fromFolder(argv[1], side, Classes);
		// sempre os mesmos pesos
		std::mt19937 random(1);
		Network network;
		network.setInputs(side * side, side, random);
		network.insertLayer(0, hidden, Network::Relu, random);
		network.insertLayer(1, Classes, Network::Softmax, random);
		network.randomize(random);
		network.save(pathFromUtf8(argv[2]));
		samples.saveSamples(pathFromUtf8(argv[2]));
		printf("%s: %d inputs, %d hidden, %d outputs, %d samples\n", argv[2], network.inputs, hidden,
			Classes, samples.size());
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "Error: %s\n", e.what());
		return 1;
	}
	return 0;
}
