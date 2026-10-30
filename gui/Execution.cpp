#include "Execution.h"
#include "InputNode.h"
#include "NeuronNode.h"
#include "OutputNode.h"
#include "../functions/Linear.h"
#include "../functions/Degree.h"
#include "../functions/Bipolar.h"
#include "../functions/Logistic.h"
#include "../functions/Hiperbolic.h"
#include "../functions/Relu.h"
#include <algorithm>
#include <cmath>

static LinearFunction linear;
static DegreeFunction degree;
static BipolarFunction bipolar;
static LogisticFunction logistic(1.0);
static HiperbolicFunction hiperbolic(1.0);
static ReluFunction relu;
// mesma ordem de NeuronNode::FunctionType; a softmax depende dos outros
// neurônios softmax e é calculada em propagate
static Function* functions[] = { &linear, &degree, &bipolar, &logistic, &hiperbolic, &relu,
	NULL };

/**
 * Derivada da função de ativação em função da saída y do neurônio. Degrau e
 * bipolar não têm derivada útil e usam 1, como na regra do perceptron. A
 * softmax também usa 1: o erro dela fica sendo a diferença para o alvo, o da
 * entropia cruzada, que não some quando a saída está longe do alvo.
 */
static double derivative(int function, double y)
{
	if(function == NeuronNode::Logistic)
		return y * (1 - y);
	if(function == NeuronNode::Hiperbolic)
		return (1 - y * y) / 2;
	if(function == NeuronNode::Relu)
		return y > 0 ? 1 : 0;
	return 1;
}

Execution::Execution(std::vector<Node*>& nodes, std::vector<Edge*>& edges): nodes(nodes),
	edges(edges)
{
	cycle = false;
}

void Execution::sort()
{
	order.clear();
	incoming.assign(nodes.size(), std::vector<int>());
	outgoing.assign(nodes.size(), std::vector<int>());
	sources.clear();
	targets.clear();
	inputs.clear();
	outputs.clear();
	neurons.clear();
	outputIndexes.clear();
	neuronIndexes.clear();
	softmax.clear();
	cycle = false;
	std::map<Node*, int> index;
	for(size_t i = 0; i < nodes.size(); i++)
		index[nodes[i]] = (int)i;
	for(size_t i = 0; i < edges.size(); i++)
	{
		int source = index[edges[i]->getFirst()], target = index[edges[i]->getSecond()];
		sources.push_back(source);
		targets.push_back(target);
		incoming[target].push_back((int)i);
		outgoing[source].push_back((int)i);
	}
	for(size_t i = 0; i < nodes.size(); i++)
	{
		Node* node = nodes[i];
		if(node->is(Node::TypeInput))
			inputs.push_back(node);
		else if(node->is(Node::TypeOutput))
		{
			outputs.push_back((OutputNode*)node);
			outputIndexes.push_back((int)i);
		}
		else
		{
			neurons.push_back((NeuronNode*)node);
			neuronIndexes.push_back((int)i);
			if(isSoftmax((int)i))
				softmax.push_back((int)i);
		}
	}
	std::vector<int> state(nodes.size(), 0);
	for(size_t i = 0; i < nodes.size(); i++)
		visit((int)i, state);
}

bool Execution::isSoftmax(int node)
{
	return nodes[node]->is(Node::TypeNeuron) &&
		((NeuronNode*)nodes[node])->getFunctionType() == NeuronNode::Softmax;
}

void Execution::visit(int node, std::vector<int>& state)
{
	// 1: visitando, 2: visitado
	if(state[node] == 2)
		return;
	if(state[node] == 1)
	{
		cycle = true;
		return;
	}
	std::vector<int> group(1, node);
	if(isSoftmax(node))
		group = softmax;
	for(int member : group)
		state[member] = 1;
	for(int member : group)
	{
		for(int edge : incoming[member])
			visit(sources[edge], state);
	}
	for(int member : group)
	{
		state[member] = 2;
		order.push_back(member);
	}
}

bool Execution::execute()
{
	sort();
	propagate();
	return !cycle;
}

double Execution::sum(int node)
{
	double sum = 0;
	for(int edge : incoming[node])
		sum += edges[edge]->getWeight() * nodes[sources[edge]]->getValue();
	return sum;
}

void Execution::propagate()
{
	for(int index : order)
	{
		Node* node = nodes[index];
		if(node->is(Node::TypeInput))
			continue;
		if(!isSoftmax(index))
		{
			double value = sum(index);
			if(node->is(Node::TypeNeuron))
			{
				NeuronNode* neuron = (NeuronNode*)node;
				value = functions[neuron->getFunctionType()]->execute(value + neuron->getBias());
			}
			node->setValue(value);
			continue;
		}
		// os neurônios softmax estão juntos na ordem, o primeiro calcula todos:
		// e^x de cada um dividido pela soma, com o maior x subtraído para e^x
		// não estourar
		if(index != softmax[0])
			continue;
		std::vector<double> values;
		double highest = -INFINITY;
		for(int member : softmax)
		{
			values.push_back(sum(member) + ((NeuronNode*)nodes[member])->getBias());
			highest = std::max(highest, values.back());
		}
		double total = 0;
		for(double& value : values)
		{
			value = exp(value - highest);
			total += value;
		}
		for(size_t i = 0; i < softmax.size(); i++)
			nodes[softmax[i]]->setValue(values[i] / total);
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

bool Execution::matches(const Sample& sample)
{
	return samples.empty() || (samples[0].inputs.size() == sample.inputs.size() &&
		samples[0].targets.size() == sample.targets.size());
}

bool Execution::remember(const Sample& sample)
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
	// erro de cada nó, pelo índice
	std::vector<double> delta(nodes.size());
	for(const Sample& sample : samples)
	{
		apply(sample);
		// das saídas para as entradas; a saída soma os nós sem função de
		// ativação e o erro dela é a própria diferença
		std::fill(delta.begin(), delta.end(), 0);
		for(size_t i = 0; i < outputs.size(); i++)
		{
			double difference = sample.targets[i] - outputs[i]->getValue();
			evaluation.squared += difference * difference;
			evaluation.absolute += fabs(difference);
			evaluation.onTarget = evaluation.onTarget &&
				fabs(difference) < OutputNode::Tolerance;
			delta[outputIndexes[i]] = difference;
		}
		for(auto it = order.rbegin(); it != order.rend(); it++)
		{
			Node* node = nodes[*it];
			if(!node->is(Node::TypeNeuron))
				continue;
			double sum = 0;
			for(int edge : outgoing[*it])
				sum += edges[edge]->getWeight() * delta[targets[edge]];
			delta[*it] = sum * derivative(((NeuronNode*)node)->getFunctionType(),
				node->getValue());
		}
		// o peso anda no sentido do erro do destino vezes o valor da origem; o
		// bias é o peso de uma entrada que vale sempre 1
		for(size_t i = 0; i < edges.size(); i++)
			gradient[i] += delta[targets[i]] * nodes[sources[i]]->getValue();
		for(size_t i = 0; i < neurons.size(); i++)
			gradient[edges.size() + i] += delta[neuronIndexes[i]];
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

Execution::TrainResult Execution::train(bool withScreen)
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
	// as saídas da softmax somam 1 e não chegam a todos os alvos em 0
	bool hasTarget = std::any_of(screen.targets.begin(), screen.targets.end(),
		[](double target) { return target != 0; });
	if(withScreen && (softmax.empty() || hasTarget))
		result.samplesChanged = remember(screen);
	else if(!matches(screen))
		samples.clear();
	result.samples = (int)samples.size();
	if(samples.empty())
	{
		result.status = NoTarget;
		return result;
	}
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
