/**
 * Treina e usa pela linha de comando as redes da interface (.bin), com as
 * amostras gravadas ao lado delas (rede.amostras.txt). Usa o mesmo código da
 * interface, sem abrir a janela.
 */
#include "../gui/NeuralNetwork.h"
#include "../gui/Pixels.h"
#include "../gui/Platform.h"
#include "Console.h"
#include <cmath>
#include <stdio.h>
#include <stdexcept>

static const char Usage[] =
	"Uso:\n"
	"  rna treinar <rede.bin> [saida.bin]\n"
	"      Treina a rede com as amostras gravadas ao lado dela, como o botão Treinar\n"
	"      da interface, e grava a rede treinada em saida.bin ou, sem ele, na própria\n"
	"      rede.\n"
	"  rna reconhecer <rede.bin> <imagem>...\n"
	"      Mostra a saída de maior valor da rede para cada imagem, como ao arrastá-la\n"
	"      para a janela. Com o dígito no começo do nome (7_3.png) mostra também os\n"
	"      acertos.\n";

static std::string nameOf(const std::string& path)
{
	return pathToUtf8(pathFromUtf8(path).filename());
}

static std::string samplesText(int count)
{
	return std::to_string(count) + (count == 1 ? " amostra" : " amostras");
}

/**
 * Lê a rede e as amostras dela, lança exceção com o arquivo que falhou
 */
static int load(NeuralNetwork& network, const std::string& path)
{
	try
	{
		network.loadFromFile(path);
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error("Falha na leitura da rede " + nameOf(path) + ": " + e.what());
	}
	try
	{
		return network.loadSamples(path);
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error("Falha na leitura das amostras de " + nameOf(path) + ": " +
			e.what());
	}
}

/**
 * Retorna 0 se a rede chegou aos alvos e 2 se não chegou
 */
static int train(const std::vector<std::string>& args)
{
	const std::string& input = args[2];
	const std::string& output = args.size() > 3 ? args[3] : input;
	NeuralNetwork network;
	if(load(network, input) == 0)
		throw std::runtime_error(nameOf(input) + " não tem amostras para treinar. Elas ficam em " +
			pathToUtf8(pathFromUtf8(input).replace_extension(".amostras.txt").filename()) +
			", gravado pelo Salvar da interface.");
	Execution::TrainResult result = network.train(false);
	std::string errors = "erro de " + Node::formatNumber(result.errorBefore, 2) + " para " +
		Node::formatNumber(result.errorAfter, 2) + ".";
	std::string iterations = std::to_string(result.iterations) +
		(result.iterations == 1 ? " iteração" : " iterações");
	std::string samples = " com " + samplesText(result.samples);
	switch(result.status)
	{
	case Execution::Trained:
		printf("Treinado%s em %s: %s\n", samples.c_str(), iterations.c_str(), errors.c_str());
		break;
	case Execution::OnTarget:
		printf("As saídas já estão iguais aos alvos nas %s, os pesos foram mantidos.\n",
			samplesText(result.samples).c_str());
		break;
	case Execution::NotConverged:
		printf("Não convergiu em %s%s: %s Treine de novo para continuar.\n", iterations.c_str(),
			samples.c_str(), errors.c_str());
		break;
	case Execution::Stalled:
		printf("O erro parou de cair em %s%s: %s Troque a função dos neurônios ou sorteie os "
			"pesos na interface.\n", iterations.c_str(), samples.c_str(), errors.c_str());
		break;
	case Execution::Invalid:
		throw std::runtime_error("A rede tem valores inválidos (nan ou inf) e não pode ser "
			"treinada. Sorteie os pesos na interface para recomeçar.");
	case Execution::HasCycle:
		throw std::runtime_error("A rede tem um ciclo e não pode ser treinada.");
	case Execution::NoOutput:
		throw std::runtime_error("A rede não tem saídas para treinar.");
	case Execution::NoTarget:
		throw std::runtime_error(nameOf(input) + " não tem amostras para treinar.");
	}
	if(result.changed || output != input)
	{
		try
		{
			network.saveToFile(output);
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error(std::string("Falha ao salvar a rede: ") + e.what());
		}
		printf("Rede salva em %s.\n", output.c_str());
	}
	bool reached = result.status == Execution::Trained || result.status == Execution::OnTarget;
	return reached ? 0 : 2;
}

/**
 * Retorna 1 se alguma imagem não pôde ser lida
 */
static int recognize(const std::vector<std::string>& args)
{
	NeuralNetwork network;
	load(network, args[2]);
	int side = network.getImageSide();
	if(side == 0)
		throw std::runtime_error("As entradas de dado de " + nameOf(args[2]) + " não formam um "
			"quadrado de pixels, como 64 para 8 x 8.");
	int labeled = 0, right = 0, failed = 0;
	for(size_t i = 3; i < args.size(); i++)
	{
		std::string name = nameOf(args[i]);
		std::vector<double> values;
		try
		{
			values = pixels::fromFile(args[i], side);
		}
		catch (const std::exception& e)
		{
			fprintf(stderr, "%s: falha na leitura da imagem: %s\n", name.c_str(), e.what());
			failed++;
			continue;
		}
		int label = pixels::labelOf(args[i]);
		bool hasLabel = network.showImage(values, label);
		int recognized = network.getRecognized();
		if(recognized < 0)
		{
			printf("%s: a rede deu valores inválidos (nan ou inf)\n", name.c_str());
			continue;
		}
		printf("%s: %d (%s)", name.c_str(), recognized,
			Node::formatNumber(network.getOutputValue(recognized), 2).c_str());
		if(hasLabel)
		{
			labeled++;
			if(recognized == label)
				right++;
			else
				printf(", esperado %d", label);
		}
		printf("\n");
	}
	if(labeled > 1)
		printf("Acertos: %d de %d (%ld%%)\n", right, labeled, lround(100.0 * right / labeled));
	return failed > 0 ? 1 : 0;
}

int main(int argc, char *argv[])
{
	std::vector<std::string> args = console::arguments(argc, argv);
	// as falhas de leitura já são explicadas nas mensagens do programa
	SetTraceLogLevel(LOG_ERROR);
	try
	{
		if(args.size() >= 3 && args.size() <= 4 && args[1] == "treinar")
			return train(args);
		if(args.size() >= 4 && args[1] == "reconhecer")
			return recognize(args);
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "%s\n", e.what());
		return 1;
	}
	fputs(Usage, stderr);
	return 1;
}
