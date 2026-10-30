#include "Execution.h"
#include "InputNode.h"
#include "NeuronNode.h"
#include "OutputNode.h"
#include <cmath>
#include <unordered_map>

Execution::Execution(std::vector<Node*>& nodes, std::vector<Edge*>& edges): nodes(nodes),
	edges(edges)
{
}

void Execution::copyIn()
{
	std::unordered_map<Node*, int> index;
	network.nodes.clear();
	network.edges.clear();
	for(size_t i = 0; i < nodes.size(); i++)
	{
		Node* node = nodes[i];
		index[node] = (int)i;
		Graph::Node data = Graph::node(node->getType());
		if(node->is(Node::TypeNeuron))
		{
			data.function = ((NeuronNode*)node)->getFunctionType();
			data.bias = ((NeuronNode*)node)->getBias();
		}
		else if(node->is(Node::TypeInput))
			data.biasInput = ((InputNode*)node)->isBias();
		else
			data.target = ((OutputNode*)node)->getTarget();
		data.x = node->getPosition().x;
		data.y = node->getPosition().y;
		data.value = node->getValue();
		network.nodes.push_back(data);
	}
	for(Edge* edge : edges)
		network.edges.push_back(Graph::Edge{ index[edge->getFirst()], index[edge->getSecond()],
			edge->getWeight(), false });
}

void Execution::copyOut(bool trained)
{
	for(size_t i = 0; i < nodes.size(); i++)
	{
		nodes[i]->setValue(network.nodes[i].value);
		if(nodes[i]->is(Node::TypeOutput))
			((OutputNode*)nodes[i])->setTarget(network.nodes[i].target);
		if(!trained || !nodes[i]->is(Node::TypeNeuron))
			continue;
		NeuronNode* neuron = (NeuronNode*)nodes[i];
		neuron->setBias(network.nodes[i].bias);
		neuron->setBiasChanged(network.nodes[i].biasChanged);
	}
	if(!trained)
		return;
	for(size_t i = 0; i < edges.size(); i++)
	{
		// o rótulo do peso só é formatado de novo se ele mudou
		if(edges[i]->getWeight() != network.edges[i].weight)
			edges[i]->setWeight(network.edges[i].weight);
		edges[i]->setChanged(network.edges[i].changed);
	}
}

bool Execution::execute()
{
	copyIn();
	bool result = network.execute();
	copyOut(false);
	return result;
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

std::vector<bool> Execution::isTrainable()
{
	copyIn();
	return network.isTrainable();
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

bool Execution::showImage(const std::vector<double>& values, int label)
{
	copyIn();
	bool result = network.showImage(values, label);
	copyOut(false);
	return result;
}

Training::Result Execution::train(bool withScreen)
{
	copyIn();
	Training::Result result = training.train(network, withScreen);
	copyOut(true);
	return result;
}

Graph& Execution::getNetwork()
{
	copyIn();
	return network;
}

int Execution::loadSamples(const std::filesystem::path& file)
{
	copyIn();
	return training.loadSamples(file, network);
}

void Execution::saveSamples(const std::filesystem::path& file)
{
	training.saveSamples(file);
}
