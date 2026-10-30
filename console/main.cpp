/**
 * Console: lê da entrada padrão uma ou mais redes no formato texto (veja o
 * README), treina cada uma com as amostras que têm saídas desejadas, mostra a
 * saída de cada amostra e escreve a rede treinada no mesmo formato.
 *
 * A rede é treinada em camadas: cada neurônio fica uma camada depois do mais
 * alto que o alimenta e cada ligação precisa ir de uma camada à seguinte, com
 * a ligação ausente valendo 0. A entrada que alimenta além da primeira camada
 * faz o papel de bias e precisa ter o mesmo valor em todas as amostras; o
 * treino ajusta o peso dela. Os neurônios não têm outro bias.
 *
 * uso: RedesNeurais < dados.txt
 */
#include "../cli/Console.h"
#include "../training/Dataset.h"
#include "../training/Network.h"
#include "../training/Trainer.h"
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <set>
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
 * Peso com só os dígitos necessários para ser lido igual em float
 */
static std::string format(float value)
{
	char str[32];
	for(int digits = 6; digits <= 9; digits++)
	{
		snprintf(str, sizeof(str), "%.*g", digits, value);
		if(strtof(str, NULL) == value)
			break;
	}
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
 * Linha de ligação: entrada, conectar ou saida
 */
struct Link
{
	enum Kind
	{
		Input,
		Neuron,
		Output
	};
	Kind kind;
	// entrada ou neurônio de origem e neurônio de destino, de 0 em diante
	int source, target;
	double weight;
};

/**
 * Rede lida da entrada, com as amostras na ordem dela
 */
struct Problem
{
	int neuronCount, inputCount;
	std::vector<int> functions;
	std::vector<double> betas;
	std::vector<Link> links;
	// neurônio de cada saída, na ordem das linhas "saida"
	std::vector<int> outputs;
	std::vector<std::vector<double> > inputs, targets;
	// a amostra tem as saídas desejadas
	std::vector<bool> targeted;
};

/**
 * Rede em camadas e onde ficou cada parte da lida
 */
struct Layered
{
	Network network;
	// camada e posição nela de cada neurônio
	std::vector<int> layer, index;
	// neurônios de cada camada, na ordem dela
	std::vector<std::vector<int> > neurons;
	// posição de cada entrada nas da rede, -1 na que faz o papel de bias
	std::vector<int> input;
	// valor da entrada que faz o papel de bias
	std::vector<double> constant;
	// o neurônio não é alimentado por uma entrada de bias e o bias fica em 0
	std::vector<std::vector<char> > fixedBiases;
};

/**
 * Lê uma rede, retorna false no fim da entrada
 */
static bool read(Reader& reader, Problem& problem)
{
	std::string first;
	if(!reader.next(first))
		return false;
	reader.back(first);
	problem.neuronCount = reader.integer("a quantidade de neurônios", 0, 1000000);
	problem.inputCount = reader.integer("a quantidade de entradas", 0, 1000000);
	int links = reader.integer("a quantidade de ligações", 0, 100000000);
	// o neurônio sem linha fica linear
	problem.functions.assign(problem.neuronCount, Network::Linear);
	problem.betas.assign(problem.neuronCount, 1);
	for(int i = 0; i < problem.neuronCount; i++)
	{
		int id = reader.integer("o número do neurônio", 1, problem.neuronCount) - 1;
		std::string name = reader.word("a função de ativação");
		int function = -1;
		for(int f = 0; f < Network::FunctionCount; f++)
		{
			if(name == functionNames[f])
				function = f;
		}
		if(function < 0)
			throw std::runtime_error("função \"" + name + "\" desconhecida, use degrau, bipolar, "
				"logistica, hiperbolica, linear, relu ou softmax.");
		problem.functions[id] = function;
		if(function == Network::Logistic || function == Network::Hiperbolic)
			problem.betas[id] = reader.number("a constante da função");
	}
	for(int i = 0; i < links; i++)
	{
		std::string action = reader.word("entrada, conectar ou saida");
		Link link = { Link::Input, 0, 0, 1 };
		if(action == "entrada")
		{
			link.source = reader.integer("o número da entrada", 1, problem.inputCount) - 1;
			link.weight = reader.number("o peso");
			link.target = reader.integer("o neurônio de destino", 1, problem.neuronCount) - 1;
		}
		else if(action == "conectar")
		{
			link.kind = Link::Neuron;
			link.source = reader.integer("o neurônio de origem", 1, problem.neuronCount) - 1;
			link.weight = reader.number("o peso");
			link.target = reader.integer("o neurônio de destino", 1, problem.neuronCount) - 1;
		}
		else if(action == "saida")
		{
			link.kind = Link::Output;
			link.source = reader.integer("o neurônio de saída", 1, problem.neuronCount) - 1;
			link.target = (int)problem.outputs.size();
			problem.outputs.push_back(link.source);
		}
		else
			throw std::runtime_error("\"" + action + "\" não é entrada, conectar nem saida.");
		problem.links.push_back(link);
	}
	if(reader.word("amostras") != "amostras")
		throw std::runtime_error("esperava amostras depois das ligações.");
	int count = reader.integer("a quantidade de amostras", 0, 100000000);
	for(int n = 0; n < count; n++)
	{
		std::vector<double> inputs, targets;
		for(int i = 0; i < problem.inputCount; i++)
			inputs.push_back(reader.number("o valor da entrada"));
		std::string word;
		bool targeted = reader.next(word) && word == "saidas";
		if(targeted)
		{
			for(size_t i = 0; i < problem.outputs.size(); i++)
				targets.push_back(reader.number("a saída desejada"));
		}
		else
		{
			if(!word.empty())
				reader.back(word);
			targets.assign(problem.outputs.size(), 0);
		}
		problem.inputs.push_back(inputs);
		problem.targets.push_back(targets);
		problem.targeted.push_back(targeted);
	}
	return true;
}

/**
 * Põe a rede em camadas, lança exceção com o motivo se ela não estiver
 */
static Layered convert(const Problem& problem)
{
	Layered result;
	int neurons = problem.neuronCount;
	std::vector<std::vector<int> > sources(neurons);
	std::set<std::pair<int, int> > linked;
	for(const Link& link : problem.links)
	{
		if(link.kind == Link::Output)
			continue;
		int source = link.kind == Link::Input ? -1 - link.source : link.source;
		if(!linked.insert(std::make_pair(source, link.target)).second)
			throw std::runtime_error("tem duas ligações iguais até o neurônio " +
				std::to_string(link.target + 1) + ".");
		if(link.kind == Link::Neuron)
			sources[link.target].push_back(link.source);
	}
	// camada do neurônio: uma depois da mais alta dos que o alimentam, 0
	// ainda não calculada e -1 calculando
	result.layer.assign(neurons, 0);
	int layers = 0;
	for(int n = 0; n < neurons; n++)
	{
		if(result.layer[n] > 0)
			continue;
		std::vector<std::pair<int, size_t> > stack(1, std::make_pair(n, (size_t)0));
		result.layer[n] = -1;
		while(!stack.empty())
		{
			int node = stack.back().first;
			size_t& next = stack.back().second;
			if(next < sources[node].size())
			{
				int source = sources[node][next++];
				if(result.layer[source] < 0)
					throw std::runtime_error("tem um ciclo e não pode ser executada.");
				if(result.layer[source] == 0)
				{
					result.layer[source] = -1;
					stack.push_back(std::make_pair(source, (size_t)0));
				}
				continue;
			}
			int layer = 1;
			for(int source : sources[node])
				layer = std::max(layer, result.layer[source] + 1);
			result.layer[node] = layer;
			layers = std::max(layers, layer);
			stack.pop_back();
		}
	}
	if(problem.outputs.empty())
		throw std::runtime_error("não tem saídas.");
	// a entrada que alimenta além da primeira camada é um bias
	result.input.assign(problem.inputCount, 0);
	result.constant.assign(problem.inputCount, 1);
	for(const Link& link : problem.links)
	{
		if(link.kind == Link::Input && result.layer[link.target] > 1)
			result.input[link.source] = -1;
	}
	Network& network = result.network;
	for(int i = 0; i < problem.inputCount; i++)
	{
		if(result.input[i] == 0)
		{
			result.input[i] = network.inputs++;
			continue;
		}
		if(!problem.inputs.empty())
			result.constant[i] = problem.inputs[0][i];
		for(const std::vector<double>& sample : problem.inputs)
		{
			if(sample[i] != result.constant[i])
				throw std::runtime_error("a entrada " + std::to_string(i + 1) + " alimenta além da "
					"primeira camada e muda de valor entre as amostras. Só uma entrada constante, "
					"que faz o papel de bias, pode ligar a outras camadas.");
		}
	}
	// a última camada na ordem das saídas, as demais na dos neurônios
	result.neurons.assign(layers, std::vector<int>());
	for(int n = 0; n < neurons; n++)
	{
		if(result.layer[n] < layers)
			result.neurons[result.layer[n] - 1].push_back(n);
	}
	for(int output : problem.outputs)
	{
		std::vector<int>& last = result.neurons.back();
		if(result.layer[output] != layers)
			throw std::runtime_error("a saída do neurônio " + std::to_string(output + 1) +
				" não vem da última camada.");
		if(std::find(last.begin(), last.end(), output) != last.end())
			throw std::runtime_error("o neurônio " + std::to_string(output + 1) +
				" tem duas saídas.");
		last.push_back(output);
	}
	for(int n = 0; n < neurons; n++)
	{
		if(result.layer[n] == layers && std::find(problem.outputs.begin(), problem.outputs.end(),
			n) == problem.outputs.end())
			throw std::runtime_error("o neurônio " + std::to_string(n + 1) + " está na última "
				"camada e não tem saída.");
	}
	result.index.assign(neurons, 0);
	for(int l = 0; l < layers; l++)
	{
		const std::vector<int>& members = result.neurons[l];
		int function = problem.functions[members[0]];
		double beta = problem.betas[members[0]];
		for(size_t j = 0; j < members.size(); j++)
		{
			result.index[members[j]] = (int)j;
			if(problem.functions[members[j]] != function || problem.betas[members[j]] != beta)
				throw std::runtime_error("os neurônios " + std::to_string(members[0] + 1) + " e " +
					std::to_string(members[j] + 1) + " estão na mesma camada e têm funções "
					"diferentes.");
		}
		Network::Layer layer = Network::layer(network.inputCount(l), (int)members.size(), function);
		layer.beta = (float)beta;
		network.layers.push_back(layer);
		result.fixedBiases.push_back(std::vector<char>(members.size(), 1));
	}
	for(const Link& link : problem.links)
	{
		if(link.kind == Link::Output)
			continue;
		int layer = result.layer[link.target] - 1, target = result.index[link.target];
		Network::Layer& weights = network.layers[layer];
		if(link.kind == Link::Input && result.input[link.source] < 0)
		{
			// a entrada constante em 0 não muda nada
			double value = result.constant[link.source];
			weights.bias[target] += (float)(link.weight * value);
			if(value != 0)
				result.fixedBiases[layer][target] = 0;
			continue;
		}
		int source = link.kind == Link::Input ? result.input[link.source] :
			result.index[link.source];
		if(link.kind == Link::Neuron && result.layer[link.source] + 1 != result.layer[link.target])
			throw std::runtime_error("a ligação do neurônio " + std::to_string(link.source + 1) +
				" ao " + std::to_string(link.target + 1) + " pula camadas.");
		weights.weights[(size_t)source * weights.size + target] = (float)link.weight;
	}
	return result;
}

/**
 * Valores das entradas da rede em camadas, sem as de bias
 */
static std::vector<float> inputsOf(const Layered& layered, const std::vector<double>& sample)
{
	std::vector<float> values(layered.network.inputs);
	for(size_t i = 0; i < sample.size(); i++)
	{
		if(layered.input[i] >= 0)
			values[layered.input[i]] = (float)sample[i];
	}
	return values;
}

/**
 * Escreve a rede treinada no formato de entrada, com as amostras: as ligações
 * na ordem lida, com os pesos novos, e depois as que o treino criou
 */
static void write(const Problem& problem, const Layered& layered)
{
	const Network& network = layered.network;
	// bias que as entradas de bias davam antes do treino
	std::vector<std::vector<float> > before;
	for(const Network::Layer& layer : network.layers)
		before.push_back(std::vector<float>(layer.size, 0));
	std::set<std::pair<int, int> > linked;
	for(const Link& link : problem.links)
	{
		if(link.kind == Link::Input && layered.input[link.source] < 0)
			before[layered.layer[link.target] - 1][layered.index[link.target]] +=
				(float)(link.weight * layered.constant[link.source]);
		else if(link.kind != Link::Output)
			linked.insert(std::make_pair(link.kind == Link::Input ? -1 - link.source : link.source,
				link.target));
	}
	std::vector<std::string> lines;
	// a primeira entrada de bias de cada neurônio leva a mudança do bias
	std::set<int> adjusted;
	for(const Link& link : problem.links)
	{
		char line[128];
		if(link.kind == Link::Output)
		{
			snprintf(line, sizeof(line), "saida    %d", link.source + 1);
			lines.push_back(line);
			continue;
		}
		const Network::Layer& layer = network.layers[layered.layer[link.target] - 1];
		int target = layered.index[link.target];
		float weight;
		if(link.kind == Link::Input && layered.input[link.source] < 0)
		{
			double value = layered.constant[link.source];
			weight = (float)link.weight;
			if(value != 0 && adjusted.insert(link.target).second)
				weight += (float)((layer.bias[target] -
					before[layered.layer[link.target] - 1][target]) / value);
		}
		else
		{
			int source = link.kind == Link::Input ? layered.input[link.source] :
				layered.index[link.source];
			weight = layer.weights[(size_t)source * layer.size + target];
		}
		snprintf(line, sizeof(line), "%s %d %s %d", link.kind == Link::Input ? "entrada " :
			"conectar", link.source + 1, format(weight).c_str(), link.target + 1);
		lines.push_back(line);
	}
	for(size_t l = 0; l < network.layers.size(); l++)
	{
		const Network::Layer& layer = network.layers[l];
		for(int i = 0; i < network.inputCount((int)l); i++)
		{
			for(int j = 0; j < layer.size; j++)
			{
				float weight = layer.weights[(size_t)i * layer.size + j];
				int target = layered.neurons[l][j];
				int source = 0;
				if(l == 0)
				{
					while(layered.input[source] != i)
						source++;
				}
				else
					source = layered.neurons[l - 1][i];
				if(weight == 0 || linked.count(std::make_pair(l == 0 ? -1 - source : source, target)))
					continue;
				char line[128];
				snprintf(line, sizeof(line), "%s %d %s %d", l == 0 ? "entrada " : "conectar",
					source + 1, format(weight).c_str(), target + 1);
				lines.push_back(line);
			}
		}
	}
	printf("%d %d %d\n", problem.neuronCount, problem.inputCount, (int)lines.size());
	for(int i = 0; i < problem.neuronCount; i++)
	{
		int function = problem.functions[i];
		printf("%d %s", i + 1, functionNames[function]);
		if(function == Network::Logistic || function == Network::Hiperbolic)
			printf(" %s", format(problem.betas[i]).c_str());
		printf("\n");
	}
	printf("\n");
	for(const std::string& line : lines)
		printf("%s\n", line.c_str());
	printf("\namostras %d\n", (int)problem.inputs.size());
	for(size_t n = 0; n < problem.inputs.size(); n++)
	{
		const std::vector<double>& inputs = problem.inputs[n];
		for(size_t i = 0; i < inputs.size(); i++)
			printf("%s%s", i > 0 ? " " : "", format(inputs[i]).c_str());
		printf("\n");
		if(!problem.targeted[n])
			continue;
		printf("saidas");
		for(double target : problem.targets[n])
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
 * cada amostra
 */
static void run(int number, const Problem& problem)
{
	printf("Rede %d: ", number);
	Layered layered;
	try
	{
		layered = convert(problem);
	}
	catch (const std::exception& e)
	{
		printf("%s\n", e.what());
		return;
	}
	Network& network = layered.network;
	Dataset samples(network.inputs, network.outputs());
	for(size_t n = 0; n < problem.inputs.size(); n++)
	{
		if(problem.targeted[n])
			samples.add(inputsOf(layered, problem.inputs[n]),
				std::vector<float>(problem.targets[n].begin(), problem.targets[n].end()));
	}
	bool changed = false;
	if(samples.empty())
		printf("as amostras não têm saídas desejadas, a rede não foi treinada.\n");
	else
	{
		Trainer trainer;
		trainer.fixedBiases = layered.fixedBiases;
		Trainer::Result result = trainer.train(network, samples);
		std::string steps = std::to_string(result.steps) + (result.epochs ?
			(result.steps == 1 ? " época" : " épocas") :
			(result.steps == 1 ? " iteração" : " iterações"));
		std::string count = samplesText(result.samples);
		std::string errors = brief(result.errorBefore) + " para " + brief(result.errorAfter);
		switch(result.status)
		{
		case Trainer::Trained:
			printf("treinada com %s em %s, erro de %s.\n", count.c_str(), steps.c_str(),
				errors.c_str());
			break;
		case Trainer::OnTarget:
			printf("as saídas já estão iguais aos alvos em %s.\n", count.c_str());
			break;
		case Trainer::NotConverged:
			printf("não convergiu em %s com %s, erro de %s. Rode de novo com a rede treinada "
				"para continuar.\n", steps.c_str(), count.c_str(), errors.c_str());
			break;
		case Trainer::Stalled:
			printf("o erro parou de cair em %s com %s, erro de %s.\n", steps.c_str(),
				count.c_str(), errors.c_str());
			break;
		case Trainer::Finished:
		case Trainer::Stopped:
			printf("treinada por %s com %s, erro médio de %s por amostra.\n", steps.c_str(),
				count.c_str(), brief(result.errorAfter).c_str());
			break;
		case Trainer::Invalid:
			printf("tem valores inválidos (nan ou inf) e não pode ser treinada.\n");
			return;
		case Trainer::NoOutput:
		case Trainer::NoSamples:
		case Trainer::Mismatch:
			printf("não tem amostras para treinar.\n");
			return;
		}
		changed = result.changed;
	}
	for(size_t n = 0; n < problem.inputs.size(); n++)
	{
		std::vector<float> outputs = network.run(inputsOf(layered, problem.inputs[n]));
		printf("Amostra %d: saídas", (int)n + 1);
		for(float output : outputs)
			printf(" %s", brief(output).c_str());
		if(problem.targeted[n])
		{
			printf(" (desejadas");
			for(double target : problem.targets[n])
				printf(" %s", brief(target).c_str());
			printf(")");
		}
		printf("\n");
	}
	if(!changed)
		return;
	printf("\nRede treinada:\n");
	write(problem, layered);
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
