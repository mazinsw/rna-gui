/**
 * Treina e usa pela linha de comando as redes da interface (.bin), com as
 * amostras gravadas ao lado delas (rede.amostras.txt) ou com um conjunto de
 * imagens, pelo mesmo treino e a mesma redução de imagens da interface.
 */
#include "../gui/Number.h"
#include "../gui/Pixels.h"
#include "../gui/Platform.h"
#include "../training/Dataset.h"
#include "../training/Network.h"
#include "../training/Trainer.h"
#include "Console.h"
#include <raylib.h>
#include <chrono>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>

static const char Usage[] =
	"Uso:\n"
	"  rna treinar <rede.bin> [dados] [--epocas N] [--saida saida.bin]\n"
	"      Treina a rede, como o botão Treinar da interface, e grava a rede treinada\n"
	"      em saida.bin ou, sem ele, na própria rede. Sem dados usa as amostras\n"
	"      gravadas ao lado dela; dados é uma pasta de imagens com o dígito no começo\n"
	"      do nome (7_3.png) ou as imagens do MNIST (train-images-idx3-ubyte ou a\n"
	"      pasta dele), com o t10k ao lado como teste. Acima de 2000 amostras treina\n"
	"      por épocas, 10 sem --epocas.\n"
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

static std::string percent(double fraction)
{
	return number::format(fraction * 100, 1) + "%";
}

/**
 * Lê a rede, lança exceção com o arquivo que falhou
 */
static Network load(const std::string& path)
{
	try
	{
		return Network::load(pathFromUtf8(path));
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error("Falha na leitura da rede " + nameOf(path) + ": " + e.what());
	}
}

/**
 * Amostras do treino e, do MNIST, o conjunto de teste. Sem dados, as gravadas
 * ao lado da rede.
 */
static Dataset loadData(const Network& network, const std::string& path, const std::string& data,
	Dataset& test)
{
	try
	{
		if(data.empty())
			return Dataset::loadSamples(pathFromUtf8(path), network.inputs, network.outputs());
		std::filesystem::path path = pathFromUtf8(data);
		if(Dataset::isIdx(path))
		{
			std::filesystem::path testPath = Dataset::testPath(path);
			if(!testPath.empty())
				test = Dataset::loadIdx(testPath);
			return Dataset::loadIdx(path);
		}
		if(network.side == 0)
			throw std::runtime_error("as entradas da rede não formam um quadrado de pixels.");
		return pixels::fromFolder(data, network.side, network.outputs());
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error("Falha na leitura das amostras de " +
			nameOf(data.empty() ? path : data) + ": " + e.what());
	}
}

/**
 * Retorna 0 se a rede chegou aos alvos ou fez as épocas e 2 se não
 */
static int train(const std::string& input, const std::string& data, const std::string& output,
	int epochs)
{
	Network network = load(input);
	Dataset test;
	Dataset samples = loadData(network, input, data, test);
	if(samples.empty())
		throw std::runtime_error(nameOf(input) + " não tem amostras para treinar. Elas ficam em " +
			pathToUtf8(Dataset::samplesPath(pathFromUtf8(input)).filename()) + ", gravado pelo "
			"Salvar da interface.");
	Trainer trainer;
	if(epochs > 0)
		trainer.epochs = epochs;
	auto start = std::chrono::steady_clock::now();
	trainer.progress = [&](const Trainer::Progress& progress)
	{
		if(!progress.epochs || progress.done < 1)
			return;
		double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() -
			start).count();
		printf("Época %d de %d: ", progress.step, progress.steps);
		if(progress.accuracy >= 0)
			printf("acerto de %s no treino", percent(progress.accuracy).c_str());
		else
			printf("erro médio de %s", number::format(progress.error, 3).c_str());
		if(progress.testAccuracy >= 0)
			printf(" e %s no teste", percent(progress.testAccuracy).c_str());
		printf(", %s s\n", number::format(seconds, 1).c_str());
		fflush(stdout);
	};
	Trainer::Result result = trainer.train(network, samples, test.empty() ? nullptr : &test);
	std::string errors = "erro de " + number::format(result.errorBefore, 2) + " para " +
		number::format(result.errorAfter, 2) + ".";
	std::string steps = std::to_string(result.steps) + (result.steps == 1 ? " iteração" :
		" iterações");
	std::string count = " com " + samplesText(result.samples);
	switch(result.status)
	{
	case Trainer::Trained:
		printf("Treinado%s em %s: %s\n", count.c_str(), steps.c_str(), errors.c_str());
		break;
	case Trainer::OnTarget:
		printf("As saídas já estão iguais aos alvos em %s, os pesos foram mantidos.\n",
			samplesText(result.samples).c_str());
		break;
	case Trainer::NotConverged:
		printf("Não convergiu em %s%s: %s Treine de novo para continuar.\n", steps.c_str(),
			count.c_str(), errors.c_str());
		break;
	case Trainer::Stalled:
		printf("O erro parou de cair em %s%s: %s Troque a função dos neurônios ou sorteie os "
			"pesos na interface.\n", steps.c_str(), count.c_str(), errors.c_str());
		break;
	case Trainer::Finished:
	case Trainer::Stopped:
		printf("Treinado por %d %s%s.\n", result.steps, result.steps == 1 ? "época" : "épocas",
			count.c_str());
		break;
	case Trainer::Invalid:
		throw std::runtime_error("A rede tem valores inválidos (nan ou inf) e não pode ser "
			"treinada. Sorteie os pesos na interface para recomeçar.");
	case Trainer::NoOutput:
		throw std::runtime_error("A rede não tem camadas para treinar.");
	case Trainer::NoSamples:
		throw std::runtime_error(nameOf(input) + " não tem amostras para treinar.");
	case Trainer::Mismatch:
		throw std::runtime_error("As amostras têm " + std::to_string(samples.inputs) +
			" entradas e " + std::to_string(samples.outputs) + " saídas e a rede, " +
			std::to_string(network.inputs) + " e " + std::to_string(network.outputs()) + ".");
	}
	if(result.changed || output != input)
	{
		try
		{
			network.save(pathFromUtf8(output));
			// as amostras de fora ficam onde estão
			if(data.empty())
				samples.saveSamples(pathFromUtf8(output));
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error(std::string("Falha ao salvar a rede: ") + e.what());
		}
		printf("Rede salva em %s.\n", output.c_str());
	}
	bool reached = result.status == Trainer::Trained || result.status == Trainer::OnTarget ||
		result.status == Trainer::Finished;
	return reached ? 0 : 2;
}

/**
 * Retorna 1 se alguma imagem não pôde ser lida
 */
static int recognize(const std::vector<std::string>& args)
{
	Network network = load(args[2]);
	if(network.side == 0)
		throw std::runtime_error("As entradas de " + nameOf(args[2]) + " não formam um quadrado de "
			"pixels, como 64 para 8 x 8.");
	if(network.layers.empty())
		throw std::runtime_error(nameOf(args[2]) + " não tem camadas.");
	int labeled = 0, right = 0, failed = 0;
	for(size_t i = 3; i < args.size(); i++)
	{
		std::string name = nameOf(args[i]);
		std::vector<double> values;
		try
		{
			values = pixels::fromFile(args[i], network.side);
		}
		catch (const std::exception& e)
		{
			fprintf(stderr, "%s: falha na leitura da imagem: %s\n", name.c_str(), e.what());
			failed++;
			continue;
		}
		std::vector<float> outputs = network.run(std::vector<float>(values.begin(), values.end()));
		int recognized = Trainer::recognized(outputs.data(), (int)outputs.size());
		if(recognized < 0)
		{
			printf("%s: a rede deu valores inválidos (nan ou inf)\n", name.c_str());
			continue;
		}
		printf("%s: %d (%s)", name.c_str(), recognized,
			number::format(outputs[recognized], 2).c_str());
		int label = pixels::labelOf(args[i]);
		if(label >= 0 && label < (int)outputs.size())
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
	console::showUtf8();
	std::vector<std::string> args = console::arguments(argc, argv);
	// as falhas de leitura já são explicadas nas mensagens do programa
	SetTraceLogLevel(LOG_ERROR);
	try
	{
		if(args.size() >= 3 && args[1] == "treinar")
		{
			std::string data, output = args[2];
			int epochs = 0;
			bool valid = true;
			for(size_t i = 3; i < args.size() && valid; i++)
			{
				if(args[i] == "--epocas" && i + 1 < args.size())
					valid = (epochs = atoi(args[++i].c_str())) > 0;
				else if(args[i] == "--saida" && i + 1 < args.size())
					output = args[++i];
				else if(data.empty() && args[i].compare(0, 2, "--") != 0)
					data = args[i];
				else
					valid = false;
			}
			if(valid)
				return train(args[2], data, output, epochs);
		}
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
