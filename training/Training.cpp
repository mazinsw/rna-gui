#include "Training.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>

/**
 * Caminho em UTF-8, para as mensagens
 */
static std::string utf8(const std::filesystem::path& path)
{
	std::u8string str = path.u8string();
	return std::string(str.begin(), str.end());
}

/**
 * Indica se todos os pesos e bias são números finitos
 */
static bool isFinite(const std::vector<double>& values)
{
	for(double value : values)
	{
		if(!std::isfinite(value))
			return false;
	}
	return true;
}

Training::Training()
{
	biases = true;
}

std::vector<double> Training::save(Graph& network)
{
	std::vector<double> values;
	for(const Graph::Edge& edge : network.edges)
		values.push_back(edge.weight);
	for(int neuron : network.neurons)
		values.push_back(network.nodes[neuron].bias);
	return values;
}

void Training::restore(Graph& network, const std::vector<double>& values)
{
	size_t count = network.edges.size();
	for(size_t i = 0; i < count; i++)
		network.edges[i].weight = values[i];
	for(size_t i = 0; i < network.neurons.size(); i++)
		network.nodes[network.neurons[i]].bias = values[count + i];
}

bool Training::markChanged(Graph& network, const std::vector<double>& values)
{
	bool changed = false;
	size_t count = network.edges.size();
	for(size_t i = 0; i < count; i++)
	{
		network.edges[i].changed = network.edges[i].weight != values[i];
		changed = changed || network.edges[i].changed;
	}
	for(size_t i = 0; i < network.neurons.size(); i++)
	{
		Graph::Node& neuron = network.nodes[network.neurons[i]];
		neuron.biasChanged = neuron.bias != values[count + i];
		changed = changed || neuron.biasChanged;
	}
	return changed;
}

Training::Sample Training::current(Graph& network)
{
	Sample sample;
	for(int input : network.inputs)
		sample.inputs.push_back(network.nodes[input].value);
	for(int output : network.outputs)
		sample.targets.push_back(network.nodes[output].target);
	return sample;
}

bool Training::matches(const Sample& sample)
{
	return samples.empty() || (samples[0].inputs.size() == sample.inputs.size() &&
		samples[0].targets.size() == sample.targets.size());
}

bool Training::remember(const Sample& sample)
{
	// as guardadas com outras entradas ou saídas não valem mais
	if(!matches(sample))
		samples.clear();
	for(Sample& other : samples)
	{
		if(other.inputs == sample.inputs)
		{
			if(other.targets == sample.targets)
				return false;
			other.targets = sample.targets;
			return true;
		}
	}
	samples.push_back(sample);
	return true;
}

void Training::apply(Graph& network, const Sample& sample)
{
	for(size_t i = 0; i < network.inputs.size(); i++)
	{
		Graph::Node& input = network.nodes[network.inputs[i]];
		if(!input.biasInput)
			input.value = sample.inputs[i];
	}
	network.propagate();
}

Training::Evaluation Training::evaluate(Graph& network, std::vector<double>& gradient)
{
	Evaluation evaluation = { 0, 0, true };
	std::vector<Graph::Node>& nodes = network.nodes;
	std::vector<Graph::Edge>& edges = network.edges;
	gradient.assign(edges.size() + network.neurons.size(), 0);
	// erro de cada nó, pelo índice
	std::vector<double> delta(nodes.size());
	for(const Sample& sample : samples)
	{
		apply(network, sample);
		// das saídas para as entradas; a saída soma os nós sem função de
		// ativação e o erro dela é a própria diferença
		std::fill(delta.begin(), delta.end(), 0);
		for(size_t i = 0; i < network.outputs.size(); i++)
		{
			double difference = sample.targets[i] - nodes[network.outputs[i]].value;
			evaluation.squared += difference * difference;
			evaluation.absolute += fabs(difference);
			evaluation.onTarget = evaluation.onTarget &&
				fabs(difference) < Graph::Tolerance;
			delta[network.outputs[i]] = difference;
		}
		for(auto it = network.order.rbegin(); it != network.order.rend(); it++)
		{
			const Graph::Node& node = nodes[*it];
			if(node.type != Graph::Neuron)
				continue;
			double sum = 0;
			for(int i = network.outgoingStart[*it]; i < network.outgoingStart[*it + 1]; i++)
			{
				const Graph::Edge& edge = edges[network.outgoingEdges[i]];
				sum += edge.weight * delta[edge.target];
			}
			delta[*it] = sum * Graph::derivative(node.function, node.beta, node.value);
		}
		// o peso anda no sentido do erro do destino vezes o valor da origem; o
		// bias é o peso de uma entrada que vale sempre 1
		for(size_t i = 0; i < edges.size(); i++)
			gradient[i] += delta[edges[i].target] * nodes[edges[i].source].value;
		for(size_t i = 0; i < network.neurons.size(); i++)
			gradient[edges.size() + i] += delta[network.neurons[i]];
	}
	return evaluation;
}

Training::Result Training::train(Graph& network, bool withScreen)
{
	Result result = { Trained, 0, 0, 0, false, 0, false };
	for(Graph::Edge& edge : network.edges)
		edge.changed = false;
	for(Graph::Node& node : network.nodes)
		node.biasChanged = false;
	if(!network.execute())
	{
		result.status = HasCycle;
		return result;
	}
	if(network.outputs.empty())
	{
		result.status = NoOutput;
		return result;
	}
	// a amostra atual volta para as entradas no fim
	Sample screen = current(network);
	// as saídas da softmax somam 1 e não chegam a todos os alvos em 0
	bool hasTarget = std::any_of(screen.targets.begin(), screen.targets.end(),
		[](double target) { return target != 0; });
	if(withScreen && (network.softmax.empty() || hasTarget))
		result.samplesChanged = remember(screen);
	else if(!matches(screen))
		samples.clear();
	result.samples = (int)samples.size();
	if(samples.empty())
	{
		result.status = NoTarget;
		return result;
	}
	std::vector<double> start = save(network), gradient;
	Evaluation evaluation = evaluate(network, gradient);
	result.errorBefore = result.errorAfter = evaluation.absolute;
	if(!std::isfinite(evaluation.squared) || !isFinite(start))
	{
		apply(network, screen);
		result.status = Invalid;
		return result;
	}
	// os bias vêm depois dos pesos
	std::vector<bool> adjustable = network.isTrainable();
	adjustable.resize(start.size(), biases);
	std::vector<double> step(start.size(), InitialStep), last(start.size(), 0), best = start;
	Evaluation lowest = evaluation;
	bool stalled = false;
	while(!evaluation.onTarget && result.iterations < MaxIterations &&
		std::isfinite(evaluation.squared))
	{
		std::vector<double> values = save(network);
		double largest = 0;
		for(size_t i = 0; i < values.size(); i++)
		{
			if(!adjustable[i] || gradient[i] == 0)
			{
				last[i] = 0;
				continue;
			}
			double direction = gradient[i] * last[i];
			if(direction > 0)
				step[i] = std::min(step[i] * 1.2, MaxStep);
			else if(direction < 0)
				step[i] /= 2;
			largest = std::max(largest, step[i]);
			// passou do mínimo: o passo menor só é dado na próxima iteração
			if(direction < 0)
			{
				last[i] = 0;
				continue;
			}
			values[i] += gradient[i] > 0 ? step[i] : -step[i];
			last[i] = gradient[i];
		}
		if(largest < MinStep)
		{
			stalled = true;
			break;
		}
		result.iterations++;
		restore(network, values);
		evaluation = evaluate(network, gradient);
		if(evaluation.squared < lowest.squared)
		{
			lowest = evaluation;
			best = values;
		}
	}
	// fica com os pesos que acertam todas as amostras, mesmo que outros
	// tenham um erro menor sem acertar; nan também volta para os melhores
	if(!evaluation.onTarget && !(evaluation.squared <= lowest.squared))
	{
		restore(network, best);
		evaluation = lowest;
	}
	apply(network, screen);
	result.errorAfter = evaluation.absolute;
	result.changed = markChanged(network, start);
	if(evaluation.onTarget)
		result.status = result.iterations == 0 ? OnTarget : Trained;
	else if(stalled)
		result.status = Stalled;
	else
		result.status = NotConverged;
	return result;
}

void Training::forgetSamples()
{
	samples.clear();
}

int Training::getSampleCount()
{
	return (int)samples.size();
}

const std::vector<Training::Sample>& Training::getSamples()
{
	return samples;
}

void Training::setSamples(const std::vector<Sample>& samples)
{
	this->samples = samples;
}

std::filesystem::path Training::samplesPath(const std::filesystem::path& network)
{
	std::filesystem::path path = network;
	path.replace_extension(".amostras.txt");
	return path;
}

/**
 * Número com ponto decimal e só os dígitos necessários para ser lido igual
 */
static std::string formatSampleValue(double value)
{
	char str[32];
	snprintf(str, sizeof(str), "%.15g", value);
	if(strtod(str, NULL) != value)
		snprintf(str, sizeof(str), "%.17g", value);
	return str;
}

void Training::saveSamples(const std::filesystem::path& network)
{
	std::filesystem::path path = samplesPath(network);
	if(samples.empty())
	{
		// as amostras esquecidas não voltam ao abrir a rede
		std::error_code error;
		if(!std::filesystem::remove(path, error) && error)
			throw std::runtime_error("Unable to remove " + utf8(path) + ".");
		return;
	}
	// grava no bloco dos arquivos do console: "amostras" e a quantidade,
	// depois os valores das entradas de cada uma e "saidas" com os alvos
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if(!file)
		throw std::runtime_error("Unable to create " + utf8(path) + ".");
	file << "amostras " << samples.size() << "\n";
	for(const Sample& sample : samples)
	{
		for(size_t i = 0; i < sample.inputs.size(); i++)
			file << (i > 0 ? " " : "") << formatSampleValue(sample.inputs[i]);
		file << "\nsaidas";
		for(double target : sample.targets)
			file << " " << formatSampleValue(target);
		file << "\n";
	}
	file.close();
	if(!file)
		throw std::runtime_error("Unable to write " + utf8(path) + ".");
}

int Training::loadSamples(const std::filesystem::path& file, const Graph& network)
{
	samples.clear();
	std::filesystem::path path = samplesPath(file);
	std::error_code error;
	if(!std::filesystem::exists(path, error))
		return 0;
	std::ifstream input(path, std::ios::binary);
	if(!input)
		throw std::runtime_error("Unable to open " + utf8(path) + ".");
	size_t inputCount = 0, outputCount = 0;
	for(const Graph::Node& node : network.nodes)
	{
		if(node.type == Graph::Input)
			inputCount++;
		else if(node.type == Graph::Output)
			outputCount++;
	}
	std::string word;
	long count;
	if(!(input >> word) || word != "amostras" || !(input >> count) || count < 0)
		throw std::runtime_error("Expected \"amostras\" and the number of samples.");
	std::vector<Sample> loaded;
	for(long n = 1; n <= count; n++)
	{
		std::runtime_error mismatch("Sample " + std::to_string(n) + " does not have the " +
			std::to_string(inputCount) + " inputs and " + std::to_string(outputCount) +
			" outputs of the network.");
		// lê os valores com ponto decimal, como os arquivos do console
		auto readValues = [&](size_t size, std::vector<double>& values)
		{
			for(size_t i = 0; i < size; i++)
			{
				char *end;
				if(!(input >> word))
					throw mismatch;
				double value = strtod(word.c_str(), &end);
				if(*end != '\0' || end == word.c_str() || !std::isfinite(value))
					throw mismatch;
				values.push_back(value);
			}
		};
		Sample sample;
		readValues(inputCount, sample.inputs);
		if(!(input >> word) || word != "saidas")
			throw mismatch;
		readValues(outputCount, sample.targets);
		loaded.push_back(sample);
	}
	if(input >> word)
		throw std::runtime_error("More values than the " + std::to_string(count) +
			" samples declared.");
	samples = loaded;
	return (int)samples.size();
}
