#include "Execution.h"
#include "InputNode.h"
#include "NeuronNode.h"
#include "OutputNode.h"
#include "../functions/Linear.h"
#include "../functions/Degree.h"
#include "../functions/Bipolar.h"
#include "../functions/Logistic.h"
#include "../functions/Hiperbolic.h"
#include <algorithm>
#include <cmath>

static LinearFunction linear;
static DegreeFunction degree;
static BipolarFunction bipolar;
static LogisticFunction logistic(1.0);
static HiperbolicFunction hiperbolic(1.0);
// mesma ordem de NeuronNode::FunctionType
static Function* functions[] = { &linear, &degree, &bipolar, &logistic, &hiperbolic };

/**
 * Derivada da função de ativação em função da saída y do neurônio. Degrau e
 * bipolar não têm derivada útil e usam 1, como na regra do perceptron.
 */
static double derivative(int function, double y)
{
	if(function == NeuronNode::Logistic)
		return y * (1 - y);
	if(function == NeuronNode::Hiperbolic)
		return (1 - y * y) / 2;
	return 1;
}

Execution::Execution(std::vector<Node*>& nodes, std::vector<Edge*>& edges): nodes(nodes),
	edges(edges)
{
	cycle = false;
}

void Execution::sort()
{
	std::map<Node*, int> state;
	order.clear();
	incoming.clear();
	outgoing.clear();
	inputs.clear();
	outputs.clear();
	neurons.clear();
	cycle = false;
	for(Edge* edge : edges)
	{
		incoming[edge->getSecond()].push_back(edge);
		outgoing[edge->getFirst()].push_back(edge);
	}
	for(Node* node : nodes)
	{
		if(node->is(Node::TypeInput))
			inputs.push_back(node);
		else if(node->is(Node::TypeOutput))
			outputs.push_back((OutputNode*)node);
		else
			neurons.push_back((NeuronNode*)node);
		visit(node, state);
	}
}

void Execution::visit(Node* node, std::map<Node*, int>& state)
{
	// 1: visitando, 2: visitado
	int& current = state[node];
	if(current == 2)
		return;
	if(current == 1)
	{
		cycle = true;
		return;
	}
	current = 1;
	for(Edge* edge : incoming[node])
		visit(edge->getFirst(), state);
	current = 2;
	order.push_back(node);
}

bool Execution::execute()
{
	sort();
	propagate();
	return !cycle;
}

void Execution::propagate()
{
	for(Node* node : order)
	{
		if(node->is(Node::TypeInput))
			continue;
		double sum = 0;
		for(Edge* edge : incoming[node])
			sum += edge->getWeight() * edge->getFirst()->getValue();
		if(node->is(Node::TypeNeuron))
		{
			NeuronNode* neuron = (NeuronNode*)node;
			sum = functions[neuron->getFunctionType()]->execute(sum + neuron->getBias());
		}
		node->setValue(sum);
	}
}

bool Execution::hasOutput()
{
	return !outputs.empty();
}

double Execution::getError()
{
	double error = 0;
	for(Node* node : nodes)
	{
		if(node->is(Node::TypeOutput))
			error += fabs(((OutputNode*)node)->getTarget() - node->getValue());
	}
	return error;
}

bool Execution::isOnTarget()
{
	for(Node* node : nodes)
	{
		if(node->is(Node::TypeOutput) && !((OutputNode*)node)->isOnTarget())
			return false;
	}
	return true;
}

std::vector<double> Execution::save()
{
	std::vector<double> values;
	for(Edge* edge : edges)
		values.push_back(edge->getWeight());
	for(NeuronNode* neuron : neurons)
		values.push_back(neuron->getBias());
	return values;
}

void Execution::restore(const std::vector<double>& values)
{
	for(size_t i = 0; i < edges.size(); i++)
		edges[i]->setWeight(values[i]);
	for(size_t i = 0; i < neurons.size(); i++)
		neurons[i]->setBias(values[edges.size() + i]);
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

bool Execution::markChanged(const std::vector<double>& values)
{
	bool changed = false;
	for(size_t i = 0; i < edges.size(); i++)
	{
		bool edgeChanged = edges[i]->getWeight() != values[i];
		edges[i]->setChanged(edgeChanged);
		changed = changed || edgeChanged;
	}
	for(size_t i = 0; i < neurons.size(); i++)
	{
		bool biasChanged = neurons[i]->getBias() != values[edges.size() + i];
		neurons[i]->setBiasChanged(biasChanged);
		changed = changed || biasChanged;
	}
	return changed;
}

std::vector<bool> Execution::isTrainable()
{
	std::map<Node*, int> count;
	for(Edge* edge : edges)
		count[edge->getSecond()]++;
	std::vector<bool> trainable;
	for(Edge* edge : edges)
	{
		Node* second = edge->getSecond();
		trainable.push_back(second->is(Node::TypeNeuron) ||
			(second->is(Node::TypeOutput) && count[second] > 1));
	}
	return trainable;
}

Execution::Sample Execution::current()
{
	Sample sample;
	for(Node* input : inputs)
		sample.inputs.push_back(input->getValue());
	for(OutputNode* output : outputs)
		sample.targets.push_back(output->getTarget());
	return sample;
}

bool Execution::remember(const Sample& sample)
{
	// as guardadas com outras entradas ou saídas não valem mais
	if(!samples.empty() && (samples[0].inputs.size() != sample.inputs.size() ||
		samples[0].targets.size() != sample.targets.size()))
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

void Execution::apply(const Sample& sample)
{
	for(size_t i = 0; i < inputs.size(); i++)
	{
		if(!((InputNode*)inputs[i])->isBias())
			inputs[i]->setValue(sample.inputs[i]);
	}
	propagate();
}

Execution::Evaluation Execution::evaluate(std::vector<double>& gradient)
{
	Evaluation evaluation = { 0, 0, true };
	gradient.assign(edges.size() + neurons.size(), 0);
	for(const Sample& sample : samples)
	{
		apply(sample);
		// erro de cada nó, das saídas para as entradas; a saída soma os nós sem
		// função de ativação e o erro dela é a própria diferença
		std::map<Node*, double> delta;
		for(size_t i = 0; i < outputs.size(); i++)
		{
			double difference = sample.targets[i] - outputs[i]->getValue();
			evaluation.squared += difference * difference;
			evaluation.absolute += fabs(difference);
			evaluation.onTarget = evaluation.onTarget &&
				fabs(difference) < OutputNode::Tolerance;
			delta[outputs[i]] = difference;
		}
		for(auto it = order.rbegin(); it != order.rend(); it++)
		{
			Node* node = *it;
			if(!node->is(Node::TypeNeuron))
				continue;
			double sum = 0;
			for(Edge* edge : outgoing[node])
				sum += edge->getWeight() * delta[edge->getSecond()];
			delta[node] = sum * derivative(((NeuronNode*)node)->getFunctionType(),
				node->getValue());
		}
		// o peso anda no sentido do erro do destino vezes o valor da origem; o
		// bias é o peso de uma entrada que vale sempre 1
		for(size_t i = 0; i < edges.size(); i++)
			gradient[i] += delta[edges[i]->getSecond()] * edges[i]->getFirst()->getValue();
		for(size_t i = 0; i < neurons.size(); i++)
			gradient[edges.size() + i] += delta[neurons[i]];
	}
	return evaluation;
}

void Execution::clearChanged()
{
	for(Edge* edge : edges)
		edge->setChanged(false);
	for(Node* node : nodes)
	{
		if(node->is(Node::TypeNeuron))
			((NeuronNode*)node)->setBiasChanged(false);
	}
}

Execution::TrainResult Execution::train()
{
	TrainResult result = { Trained, 0, 0, 0, false, 0, false };
	clearChanged();
	if(!execute())
	{
		result.status = HasCycle;
		return result;
	}
	if(!hasOutput())
	{
		result.status = NoOutput;
		return result;
	}
	// a amostra da tela volta para as entradas no fim
	Sample screen = current();
	result.samplesChanged = remember(screen);
	result.samples = (int)samples.size();
	std::vector<double> start = save(), gradient;
	Evaluation evaluation = evaluate(gradient);
	result.errorBefore = result.errorAfter = evaluation.absolute;
	if(!std::isfinite(evaluation.squared) || !isFinite(start))
	{
		apply(screen);
		result.status = Invalid;
		return result;
	}
	// os bias, depois dos pesos, sempre são ajustados
	std::vector<bool> adjustable = isTrainable();
	adjustable.resize(start.size(), true);
	std::vector<double> step(start.size(), InitialStep), last(start.size(), 0), best = start;
	Evaluation lowest = evaluation;
	bool stalled = false;
	while(!evaluation.onTarget && result.iterations < MaxIterations &&
		std::isfinite(evaluation.squared))
	{
		std::vector<double> values = save();
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
		restore(values);
		evaluation = evaluate(gradient);
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
		restore(best);
		evaluation = lowest;
	}
	apply(screen);
	result.errorAfter = evaluation.absolute;
	result.changed = markChanged(start);
	if(evaluation.onTarget)
		result.status = result.iterations == 0 ? OnTarget : Trained;
	else if(stalled)
		result.status = Stalled;
	else
		result.status = NotConverged;
	return result;
}
