/**
 * Testes do treino, sem janela: as derivadas contra a derivada numérica, a
 * convergência dos exemplos, os arquivos e, com a pasta do MNIST, o acerto e
 * o tempo das épocas.
 *
 * uso: check [pasta do MNIST]
 */
#include "../training/Dataset.h"
#include "../training/Graph.h"
#include "../training/Network.h"
#include "../training/Trainer.h"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <map>
#include <random>
#include <stdio.h>
#include <string>

static int failures = 0;

static void check(bool ok, const std::string& text)
{
	printf("%s %s\n", ok ? "ok    " : "FALHOU", text.c_str());
	if(!ok)
		failures++;
}

static const char *functionNames[] = { "linear", "degrau", "bipolar", "logística",
	"hiperbólica", "ReLU", "softmax" };

/**
 * Erro que o treino reduz: entropia cruzada com softmax na última camada e
 * metade da soma dos quadrados nas demais
 */
static double loss(const Network& network, const Dataset& samples)
{
	double total = 0;
	bool softmax = network.layers.back().function == Network::Softmax;
	for(int s = 0; s < samples.size(); s++)
	{
		std::vector<float> output = network.run(samples.input(s));
		for(int o = 0; o < samples.outputs; o++)
		{
			double target = samples.target(s)[o];
			if(softmax)
				total -= target * log(output[o]);
			else
				total += (target - output[o]) * (target - output[o]) / 2;
		}
	}
	return total;
}

/**
 * Compara o gradiente do treino com a derivada numérica de cada peso e bias
 */
static void checkGradient(int hidden, int output)
{
	std::mt19937 random(7);
	std::uniform_real_distribution<float> uniform(-1, 1);
	Network network;
	network.inputs = 3;
	network.layers.push_back(Network::layer(3, 4, hidden));
	network.layers.push_back(Network::layer(4, 3, output));
	for(Network::Layer& layer : network.layers)
	{
		for(float& weight : layer.weights)
			weight = uniform(random);
		for(float& bias : layer.bias)
			bias = uniform(random);
	}
	Dataset samples(3, 3);
	for(int s = 0; s < 5; s++)
	{
		std::vector<float> input = { uniform(random), uniform(random), uniform(random) };
		std::vector<float> target = { 0, 0, 0 };
		if(output == Network::Softmax)
			target[s % 3] = 1;
		else
			target = { uniform(random), uniform(random), uniform(random) };
		samples.add(input, target);
	}
	Trainer trainer;
	Network gradient = trainer.gradient(network, samples);
	double worst = 0;
	const float h = 1e-2f;
	for(size_t l = 0; l < network.layers.size(); l++)
	{
		for(int kind = 0; kind < 2; kind++)
		{
			std::vector<float>& values = kind == 0 ? network.layers[l].weights : network.layers[l].bias;
			const std::vector<float>& analytic = kind == 0 ? gradient.layers[l].weights :
				gradient.layers[l].bias;
			for(size_t i = 0; i < values.size(); i++)
			{
				float value = values[i];
				values[i] = value + h;
				double up = loss(network, samples);
				values[i] = value - h;
				double down = loss(network, samples);
				values[i] = value;
				// o gradiente do treino é o sentido que reduz o erro
				double numeric = -(up - down) / (2 * h);
				double difference = fabs(numeric - analytic[i]) /
					std::max(1.0, std::max(fabs(numeric), fabs((double)analytic[i])));
				worst = std::max(worst, difference);
			}
		}
	}
	check(worst < 2e-3, std::string("derivadas com ") + functionNames[hidden] + " na oculta e " +
		functionNames[output] + " na saída (diferença " + std::to_string(worst) + ")");
}

static Dataset truthTable(int a, int b, int c, int d)
{
	Dataset samples(2, 1);
	samples.add({ 0, 0 }, { (float)a });
	samples.add({ 0, 1 }, { (float)b });
	samples.add({ 1, 0 }, { (float)c });
	samples.add({ 1, 1 }, { (float)d });
	return samples;
}

/**
 * Treina a rede de sizes camadas com a função em seeds sorteios dos pesos e
 * retorna quantas chegaram aos alvos
 */
static int converged(const Dataset& samples, const std::vector<int>& sizes, int function,
	int seeds)
{
	int count = 0;
	for(int seed = 1; seed <= seeds; seed++)
	{
		std::mt19937 random(seed);
		Network network;
		network.inputs = samples.inputs;
		for(int size : sizes)
			network.layers.push_back(Network::layer(network.inputCount((int)network.layers.size()), size,
				function));
		network.randomize(random);
		Trainer trainer;
		Trainer::Status status = trainer.train(network, samples).status;
		if(status == Trainer::Trained || status == Trainer::OnTarget)
			count++;
	}
	return count;
}

/**
 * Compara as saídas da rede em grafo com as da importada em camadas, com
 * entradas sorteadas
 */
static void checkImport(const std::string& path)
{
	std::map<std::string, std::vector<float> > truth = { { "and.bin", { 0, 0, 0, 1 } },
		{ "or.bin", { 0, 1, 1, 1 } }, { "xor.bin", { 0, 1, 1, 0 } } };
	std::string name = std::filesystem::path(path).filename().string();
	try
	{
		Graph graph = Graph::load(path);
		Network network = Network::load(path);
		std::mt19937 random(3);
		std::uniform_real_distribution<float> uniform(0, 1);
		double worst = 0;
		for(int sample = 0; sample < 20; sample++)
		{
			std::vector<float> input;
			for(Graph::Node& node : graph.nodes)
			{
				if(node.type != Graph::Input || node.biasInput)
					continue;
				// bits nas redes pequenas, como na interface
				node.value = network.inputs < 16 ? (float)(int)(uniform(random) * 2) : uniform(random);
				input.push_back((float)node.value);
			}
			graph.execute();
			std::vector<float> output = network.run(input);
			size_t o = 0;
			for(Graph::Node& node : graph.nodes)
			{
				if(node.type != Graph::Output)
					continue;
				worst = std::max(worst, o < output.size() ? fabs(node.value - output[o]) : 1.0);
				o++;
			}
			if(o != output.size())
				worst = 1;
		}
		check(worst < 1e-4, "importa " + name + " com as saídas da rede em grafo (diferença " +
			std::to_string(worst) + ")");
		// as redes antigas dos exemplos já estavam treinadas
		std::vector<float> table = truth[name];
		bool right = network.outputs() == 1;
		for(int i = 0; right && i < 4; i++)
			right = network.run({ (float)(i / 2), (float)(i % 2) })[0] == table[i];
		check(right, "a " + name + " importada calcula a tabela-verdade");
	}
	catch (const std::exception& e)
	{
		check(false, "importa " + name + ": " + e.what());
	}
}

static void checkFiles()
{
	std::filesystem::path folder = std::filesystem::temp_directory_path() / "rna-check";
	std::filesystem::create_directories(folder);
	std::filesystem::path file = folder / "rede.bin";
	std::mt19937 random(5);
	Network network;
	network.setInputs(16, 4, random);
	network.insertLayer(0, 5, Network::Relu, random);
	network.insertLayer(1, 3, Network::Softmax, random);
	network.layers[0].beta = 0.5f;
	network.save(file);
	Network loaded = Network::load(file);
	bool same = loaded.inputs == network.inputs && loaded.side == network.side &&
		loaded.layers.size() == network.layers.size();
	for(size_t l = 0; same && l < network.layers.size(); l++)
		same = loaded.layers[l].size == network.layers[l].size &&
			loaded.layers[l].function == network.layers[l].function &&
			loaded.layers[l].beta == network.layers[l].beta &&
			loaded.layers[l].weights == network.layers[l].weights &&
			loaded.layers[l].bias == network.layers[l].bias;
	check(same, "grava e lê a rede no formato 2.0");
	Dataset samples(16, 3);
	std::uniform_real_distribution<float> uniform(-2, 2);
	for(int s = 0; s < 4; s++)
	{
		std::vector<float> input, target(3, 0);
		for(int i = 0; i < 16; i++)
			input.push_back(uniform(random));
		target[s % 3] = 1;
		samples.add(input, target);
	}
	samples.saveSamples(file);
	Dataset read = Dataset::loadSamples(file, 16, 3);
	same = read.size() == samples.size();
	for(int s = 0; same && s < samples.size(); s++)
		same = read.input(s) == samples.input(s) && read.label(s) == samples.label(s);
	check(same, "grava e lê as amostras");
	std::filesystem::remove_all(folder);
}

static void checkMnist(const std::string& folder)
{
	Dataset train = Dataset::loadIdx(folder);
	Dataset test = Dataset::loadIdx(Dataset::testPath(folder));
	std::mt19937 random(1);
	Network network;
	network.setInputs(784, 28, random);
	network.insertLayer(0, 256, Network::Relu, random);
	network.insertLayer(1, 10, Network::Softmax, random);
	Trainer trainer;
	trainer.epochs = 7;
	auto start = std::chrono::steady_clock::now();
	trainer.progress = [&](const Trainer::Progress& progress)
	{
		if(progress.done < 1)
			return;
		double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() -
			start).count();
		printf("       época %d: %.1f%% no treino, %.2f%% no teste, %.2f s\n", progress.step,
			progress.accuracy * 100, progress.testAccuracy * 100, seconds / progress.step);
	};
	Trainer::Result result = trainer.train(network, train, &test);
	double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() -
		start).count();
	check(result.testAccuracy >= 0.955, "MNIST 784-256-10 em 7 épocas acerta " +
		std::to_string(result.testAccuracy * 100) + "% do teste");
	check(seconds / 7 <= 1.2, "MNIST leva " + std::to_string(seconds / 7) + " s por época");
}

int main(int argc, char *argv[])
{
	try
	{
		int functions[] = { Network::Linear, Network::Logistic, Network::Hiperbolic, Network::Relu };
		for(int hidden : functions)
		{
			checkGradient(hidden, Network::Linear);
			checkGradient(hidden, Network::Softmax);
		}
		checkGradient(Network::Relu, Network::Logistic);
		checkGradient(Network::Logistic, Network::Hiperbolic);
		check(converged(truthTable(0, 0, 0, 1), { 1 }, Network::Degrau, 10) == 10,
			"AND com degrau converge nos 10 sorteios");
		check(converged(truthTable(0, 1, 1, 1), { 1 }, Network::Degrau, 10) == 10,
			"OR com degrau converge nos 10 sorteios");
		int xorCount = converged(truthTable(0, 1, 1, 0), { 2, 1 }, Network::Logistic, 10);
		check(xorCount >= 3, "XOR com logística converge em " + std::to_string(xorCount) + " de 10 sorteios");
		Network digits = Network::load("exemplos/digitos.bin");
		Dataset samples = Dataset::loadSamples("exemplos/digitos.bin", digits.inputs,
			digits.outputs());
		Trainer trainer;
		Trainer::Result result = trainer.train(digits, samples);
		check(result.status == Trainer::Trained, "dígitos 8 x 8 chegam aos alvos em " +
			std::to_string(result.steps) + " iterações");
		for(const char *name : { "and", "or", "xor" })
			checkImport(std::string("test/legacy/") + name + ".bin");
		checkFiles();
		if(argc > 1)
			checkMnist(argv[1]);
	}
	catch (const std::exception& e)
	{
		check(false, e.what());
	}
	if(failures > 0)
		printf("%d falhas\n", failures);
	return failures > 0 ? 1 : 0;
}
