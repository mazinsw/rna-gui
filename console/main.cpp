/**
 * Console: lê da entrada padrão uma ou mais redes no formato texto (veja o
 * README), treina cada uma com as amostras que têm saídas desejadas, mostra a
 * saída de cada amostra e escreve a rede treinada no mesmo formato.
 *
 * uso: RedesNeurais < dados.txt
 */
#include "../cli/Console.h"
#include "../training/Network.h"
#include "../training/Training.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * Palavras da entrada, sem os comentários, que vão de ";" ao fim da linha
 */
class Reader
{
private:
	std::istream& in;
	std::istringstream line;
	std::string pending;
	bool hasPending;
public:
	Reader(std::istream& in): in(in), hasPending(false) {}

	/**
	 * Próxima palavra, retorna false no fim da entrada
	 */
	bool next(std::string& word)
	{
		if(hasPending)
		{
			word = pending;
			hasPending = false;
			return true;
		}
		while(!(line >> word))
		{
			std::string text;
			if(!std::getline(in, text))
				return false;
			text = text.substr(0, text.find(';'));
			line.clear();
			line.str(text);
		}
		return true;
	}

	/**
	 * Devolve a palavra para a próxima leitura
	 */
	void back(const std::string& word)
	{
		pending = word;
		hasPending = true;
	}

	std::string word(const char *expected)
	{
		std::string word;
		if(!next(word))
			throw std::runtime_error(std::string("fim da entrada, esperava ") + expected + ".");
		return word;
	}

	double number(const char *expected)
	{
		std::string text = word(expected);
		char *end;
		double value = strtod(text.c_str(), &end);
		if(*end != '\0' || end == text.c_str())
			throw std::runtime_error("\"" + text + "\" não é um número, esperava " + expected +
				".");
		return value;
	}

	int integer(const char *expected, int low, int high)
	{
		double value = number(expected);
		if(value != (int)value || value < low || value > high)
			throw std::runtime_error(std::string(expected) + " fora de " + std::to_string(low) +
				" a " + std::to_string(high) + ".");
		return (int)value;
	}
};

static const char *functionNames[] = { "linear", "degrau", "bipolar", "logistica",
	"hiperbolica", "relu", "softmax" };

/**
 * Número com ponto decimal e só os dígitos necessários para ser lido igual
 */
static std::string format(double value)
{
	char str[32];
	snprintf(str, sizeof(str), "%.15g", value);
	if(strtod(str, NULL) != value)
		snprintf(str, sizeof(str), "%.17g", value);
	return str;
}

/**
 * Número com 6 algarismos, para ler
 */
static std::string brief(double value)
{
	char str[32];
	snprintf(str, sizeof(str), "%g", value);
	return str;
}

/**
 * Rede lida da entrada, com as amostras na ordem dela
 */
struct Problem
{
	Network network;
	int neuronCount, inputCount;
	std::vector<Training::Sample> samples;
	// a amostra tem as saídas desejadas
	std::vector<bool> targeted;
};

/**
 * Lê uma rede, retorna false no fim da entrada. As entradas ficam no começo da
 * lista de nós, depois os neurônios e depois as saídas, na ordem das linhas
 * "saida".
 */
static bool read(Reader& reader, Problem& problem)
{
	std::string first;
	if(!reader.next(first))
		return false;
	reader.back(first);
	Network& network = problem.network;
	problem.neuronCount = reader.integer("a quantidade de neurônios", 0, 1000000);
	problem.inputCount = reader.integer("a quantidade de entradas", 0, 1000000);
	int links = reader.integer("a quantidade de ligações", 0, 100000000);
	for(int i = 0; i < problem.inputCount; i++)
		network.nodes.push_back(Network::node(Network::Input));
	for(int i = 0; i < problem.neuronCount; i++)
		network.nodes.push_back(Network::node(Network::Neuron));
	auto neuron = [&](int id) { return problem.inputCount + id - 1; };
	for(int i = 0; i < problem.neuronCount; i++)
	{
		int id = reader.integer("o número do neurônio", 1, problem.neuronCount);
		std::string name = reader.word("a função de ativação");
		Network::Node& node = network.nodes[neuron(id)];
		node.function = -1;
		for(int f = 0; f < Network::FunctionCount; f++)
		{
			if(name == functionNames[f])
				node.function = f;
		}
		if(node.function < 0)
			throw std::runtime_error("função \"" + name + "\" desconhecida, use degrau, bipolar, "
				"logistica, hiperbolica, linear, relu ou softmax.");
		if(node.function == Network::Logistic || node.function == Network::Hiperbolic)
			node.beta = reader.number("a constante da função");
	}
	for(int i = 0; i < links; i++)
	{
		std::string action = reader.word("entrada, conectar ou saida");
		if(action == "entrada")
		{
			int input = reader.integer("o número da entrada", 1, problem.inputCount) - 1;
			double weight = reader.number("o peso");
			int target = neuron(reader.integer("o neurônio de destino", 1, problem.neuronCount));
			network.edges.push_back({ input, target, weight, false });
		}
		else if(action == "conectar")
		{
			int source = neuron(reader.integer("o neurônio de origem", 1, problem.neuronCount));
			double weight = reader.number("o peso");
			int target = neuron(reader.integer("o neurônio de destino", 1, problem.neuronCount));
			network.edges.push_back({ source, target, weight, false });
		}
		else if(action == "saida")
		{
			int source = neuron(reader.integer("o neurônio de saída", 1, problem.neuronCount));
			network.nodes.push_back(Network::node(Network::Output));
			network.edges.push_back({ source, (int)network.nodes.size() - 1, 1, false });
		}
		else
			throw std::runtime_error("\"" + action + "\" não é entrada, conectar nem saida.");
	}
	int outputCount = (int)network.nodes.size() - problem.inputCount - problem.neuronCount;
	if(reader.word("amostras") != "amostras")
		throw std::runtime_error("esperava amostras depois das ligações.");
	int count = reader.integer("a quantidade de amostras", 0, 100000000);
	for(int n = 0; n < count; n++)
	{
		Training::Sample sample;
		for(int i = 0; i < problem.inputCount; i++)
			sample.inputs.push_back(reader.number("o valor da entrada"));
		std::string word;
		bool targeted = reader.next(word) && word == "saidas";
		if(targeted)
		{
			for(int i = 0; i < outputCount; i++)
				sample.targets.push_back(reader.number("a saída desejada"));
		}
		else
		{
			if(!word.empty())
				reader.back(word);
			sample.targets.assign(outputCount, 0);
		}
		problem.samples.push_back(sample);
		problem.targeted.push_back(targeted);
	}
	return true;
}

/**
 * Escreve a rede no formato de entrada, com as amostras
 */
static void write(Problem& problem)
{
	Network& network = problem.network;
	printf("%d %d %d\n", problem.neuronCount, problem.inputCount, (int)network.edges.size());
	for(int i = 0; i < problem.neuronCount; i++)
	{
		const Network::Node& node = network.nodes[problem.inputCount + i];
		printf("%d %s", i + 1, functionNames[node.function]);
		if(node.function == Network::Logistic || node.function == Network::Hiperbolic)
			printf(" %s", format(node.beta).c_str());
		printf("\n");
	}
	printf("\n");
	auto id = [&](int node) { return node - problem.inputCount + 1; };
	for(const Network::Edge& edge : network.edges)
	{
		if(network.nodes[edge.target].type == Network::Output)
			printf("saida    %d\n", id(edge.source));
		else if(network.nodes[edge.source].type == Network::Input)
			printf("entrada  %d %s %d\n", edge.source + 1, format(edge.weight).c_str(),
				id(edge.target));
		else
			printf("conectar %d %s %d\n", id(edge.source), format(edge.weight).c_str(),
				id(edge.target));
	}
	printf("\namostras %d\n", (int)problem.samples.size());
	for(size_t n = 0; n < problem.samples.size(); n++)
	{
		const Training::Sample& sample = problem.samples[n];
		for(size_t i = 0; i < sample.inputs.size(); i++)
			printf("%s%s", i > 0 ? " " : "", format(sample.inputs[i]).c_str());
		printf("\n");
		if(!problem.targeted[n])
			continue;
		printf("saidas");
		for(double target : sample.targets)
			printf(" %s", format(target).c_str());
		printf("\n");
	}
}

static std::string samplesText(int count)
{
	return std::to_string(count) + (count == 1 ? " amostra" : " amostras");
}

/**
 * Treina a rede com as amostras que têm saídas desejadas e mostra a saída de
 * cada amostra. O console não tem bias no neurônio, só os pesos são treinados.
 */
static void run(int number, Problem& problem)
{
	Network& network = problem.network;
	Training training;
	training.biases = false;
	std::vector<Training::Sample> targeted;
	for(size_t n = 0; n < problem.samples.size(); n++)
	{
		if(problem.targeted[n])
			targeted.push_back(problem.samples[n]);
	}
	training.setSamples(targeted);
	printf("Rede %d: ", number);
	bool changed = false;
	if(targeted.empty())
	{
		if(!network.execute())
		{
			printf("tem um ciclo e não pode ser executada.\n");
			return;
		}
		printf("as amostras não têm saídas desejadas, a rede não foi treinada.\n");
	}
	else
	{
		Training::Result result = training.train(network, false);
		std::string iterations = std::to_string(result.iterations) +
			(result.iterations == 1 ? " iteração" : " iterações");
		std::string samples = samplesText(result.samples);
		std::string errors = brief(result.errorBefore) + " para " + brief(result.errorAfter);
		switch(result.status)
		{
		case Training::Trained:
			printf("treinada com %s em %s, erro de %s.\n", samples.c_str(), iterations.c_str(),
				errors.c_str());
			break;
		case Training::OnTarget:
			printf("as saídas já estão iguais aos alvos em %s.\n", samples.c_str());
			break;
		case Training::NotConverged:
			printf("não convergiu em %s com %s, erro de %s. Rode de novo com a rede treinada "
				"para continuar.\n", iterations.c_str(), samples.c_str(), errors.c_str());
			break;
		case Training::Stalled:
			printf("o erro parou de cair em %s com %s, erro de %s.\n", iterations.c_str(),
				samples.c_str(), errors.c_str());
			break;
		case Training::Invalid:
			printf("tem valores inválidos (nan ou inf) e não pode ser treinada.\n");
			return;
		case Training::HasCycle:
			printf("tem um ciclo e não pode ser executada.\n");
			return;
		case Training::NoOutput:
			printf("não tem saídas.\n");
			return;
		case Training::NoTarget:
			printf("não tem amostras para treinar.\n");
			return;
		}
		changed = result.changed;
	}
	for(size_t n = 0; n < problem.samples.size(); n++)
	{
		const Training::Sample& sample = problem.samples[n];
		for(int i = 0; i < problem.inputCount; i++)
			network.nodes[i].value = sample.inputs[i];
		network.execute();
		printf("Amostra %d: saídas", (int)n + 1);
		size_t first = problem.inputCount + problem.neuronCount;
		for(size_t i = first; i < network.nodes.size(); i++)
			printf(" %s", brief(network.nodes[i].value).c_str());
		if(problem.targeted[n])
		{
			printf(" (desejadas");
			for(double target : sample.targets)
				printf(" %s", brief(target).c_str());
			printf(")");
		}
		printf("\n");
	}
	if(!changed)
		return;
	printf("\nRede treinada:\n");
	write(problem);
}

int main()
{
	console::showUtf8();
	Reader reader(std::cin);
	for(int number = 1; ; number++)
	{
		Problem problem;
		try
		{
			if(!read(reader, problem))
				break;
		}
		catch (const std::exception& e)
		{
			fprintf(stderr, "Erro na rede %d: %s\n", number, e.what());
			return 1;
		}
		if(number > 1)
			printf("\n");
		run(number, problem);
	}
	return 0;
}
