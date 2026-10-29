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
			sum = functions[((NeuronNode*)node)->getFunctionType()]->execute(sum);
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

bool Execution::step()
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
	// os pesos das conexões com as saídas ficam fixos
	std::vector<double> weights;
	for(Edge* edge : edges)
	{
		double weight = edge->getWeight();
		if(edge->getSecond()->is(Node::TypeNeuron))
			weight += LearningRate * delta[edge->getSecond()] * edge->getFirst()->getValue();
		if(!std::isfinite(weight))
			return false;
		weights.push_back(weight);
	}
	for(size_t i = 0; i < edges.size(); i++)
	{
		if(weights[i] == edges[i]->getWeight())
			continue;
		edges[i]->setWeight(weights[i]);
		edges[i]->setChanged(true);
	}
	return true;
}

Execution::TrainResult Execution::train()
{
	TrainResult result = { Trained, 0, 0, 0 };
	for(Edge* edge : edges)
		edge->setChanged(false);
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
	result.errorBefore = getError();
	while(!isOnTarget() && result.iterations < MaxIterations && step())
	{
		execute();
		result.iterations++;
	}
	result.errorAfter = getError();
	if(!isOnTarget())
		result.status = NotConverged;
	else if(result.iterations == 0)
		result.status = OnTarget;
	return result;
}
