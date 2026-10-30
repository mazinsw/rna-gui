#include "Execution.h"
#include "NeuronNode.h"
#include "OutputNode.h"
#include "../functions/Linear.h"
#include "../functions/Degree.h"
#include "../functions/Bipolar.h"
#include "../functions/Logistic.h"
#include "../functions/Hiperbolic.h"
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
	cycle = false;
	for(Edge* edge : edges)
	{
		incoming[edge->getSecond()].push_back(edge);
		outgoing[edge->getFirst()].push_back(edge);
	}
	for(Node* node : nodes)
		visit(node, state);
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
	return !cycle;
}

bool Execution::hasOutput()
{
	for(Node* node : nodes)
	{
		if(node->is(Node::TypeOutput))
			return true;
	}
	return false;
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

double Execution::squaredError()
{
	double error = 0;
	for(Node* node : nodes)
	{
		if(!node->is(Node::TypeOutput))
			continue;
		double difference = ((OutputNode*)node)->getTarget() - node->getValue();
		error += difference * difference;
	}
	return error;
}

Execution::Snapshot Execution::save()
{
	Snapshot snapshot;
	for(Edge* edge : edges)
		snapshot.weights.push_back(edge->getWeight());
	for(Node* node : nodes)
	{
		if(node->is(Node::TypeNeuron))
			snapshot.biases.push_back(((NeuronNode*)node)->getBias());
	}
	return snapshot;
}

void Execution::restore(const Snapshot& snapshot)
{
	for(size_t i = 0; i < edges.size(); i++)
		edges[i]->setWeight(snapshot.weights[i]);
	size_t i = 0;
	for(Node* node : nodes)
	{
		if(node->is(Node::TypeNeuron))
			((NeuronNode*)node)->setBias(snapshot.biases[i++]);
	}
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

bool Execution::markChanged(const Snapshot& snapshot)
{
	bool changed = false;
	for(size_t i = 0; i < edges.size(); i++)
	{
		bool edgeChanged = edges[i]->getWeight() != snapshot.weights[i];
		edges[i]->setChanged(edgeChanged);
		changed = changed || edgeChanged;
	}
	size_t i = 0;
	for(Node* node : nodes)
	{
		if(!node->is(Node::TypeNeuron))
			continue;
		NeuronNode* neuron = (NeuronNode*)node;
		bool biasChanged = neuron->getBias() != snapshot.biases[i++];
		neuron->setBiasChanged(biasChanged);
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

bool Execution::step(double rate)
{
	// erro de cada neurônio, das saídas para as entradas
	std::map<Node*, double> delta;
	for(auto it = order.rbegin(); it != order.rend(); it++)
	{
		Node* node = *it;
		if(!node->is(Node::TypeNeuron))
			continue;
		double gradient = 0;
		for(Edge* edge : outgoing[node])
		{
			Node* next = edge->getSecond();
			if(next->is(Node::TypeOutput))
				gradient += edge->getWeight() *
					(((OutputNode*)next)->getTarget() - next->getValue());
			else if(next->is(Node::TypeNeuron))
				gradient += edge->getWeight() * delta[next];
		}
		delta[node] = gradient * derivative(((NeuronNode*)node)->getFunctionType(),
			node->getValue());
	}
	std::vector<bool> trainable = isTrainable();
	std::vector<double> weights;
	for(size_t i = 0; i < edges.size(); i++)
	{
		Edge* edge = edges[i];
		Node* second = edge->getSecond();
		double weight = edge->getWeight();
		// a saída soma os nós sem função de ativação, o erro dela é o próprio
		// delta
		if(trainable[i] && second->is(Node::TypeOutput))
			weight += rate * (((OutputNode*)second)->getTarget() - second->getValue()) *
				edge->getFirst()->getValue();
		else if(trainable[i])
			weight += rate * delta[second] * edge->getFirst()->getValue();
		if(!std::isfinite(weight))
			return false;
		weights.push_back(weight);
	}
	// o bias é o peso de uma entrada que vale sempre 1
	std::map<Node*, double> biases;
	for(auto& item : delta)
	{
		double bias = ((NeuronNode*)item.first)->getBias() + rate * item.second;
		if(!std::isfinite(bias))
			return false;
		biases[item.first] = bias;
	}
	for(size_t i = 0; i < edges.size(); i++)
		edges[i]->setWeight(weights[i]);
	for(auto& item : biases)
		((NeuronNode*)item.first)->setBias(item.second);
	return true;
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
	TrainResult result = { Trained, 0, 0, 0, false };
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
	result.errorBefore = result.errorAfter = getError();
	Snapshot start = save();
	if(!std::isfinite(result.errorBefore) || !isFinite(start.weights) ||
		!isFinite(start.biases))
	{
		result.status = Invalid;
		return result;
	}
	double rate = LearningRate;
	double error = squaredError();
	while(!isOnTarget() && result.iterations < MaxIterations && rate >= MinLearningRate)
	{
		result.iterations++;
		Snapshot previous = save();
		if(step(rate))
		{
			execute();
			// nan também é desfeito
			double next = squaredError();
			if(next <= error)
			{
				if(next < error)
					rate *= 1.05;
				error = next;
				continue;
			}
			restore(previous);
			execute();
		}
		rate /= 2;
	}
	result.errorAfter = getError();
	result.changed = markChanged(start);
	if(isOnTarget())
		result.status = result.iterations == 0 ? OnTarget : Trained;
	else if(rate < MinLearningRate)
		result.status = Stalled;
	else
		result.status = NotConverged;
	return result;
}
