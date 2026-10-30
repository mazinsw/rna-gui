#include "Trainer.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
// amostras calculadas de uma vez ao medir o acerto
#define ACCURACY_CHUNK 500

Trainer::Trainer(): stop(false)
{
	epochs = 10;
	rate = 0.1f;
	batch = 128;
	seed = 1;
}

bool Trainer::byEpochs(int count)
{
	return count > MaxFullBatch;
}

int Trainer::recognized(const float* outputs, int count)
{
	int recognized = -1;
	for(int o = 0; o < count; o++)
	{
		if(!std::isfinite(outputs[o]))
			return -1;
		if(recognized < 0 || outputs[o] > outputs[recognized])
			recognized = o;
	}
	return recognized;
}

double Trainer::accuracy(const Model& model, const Dataset& samples)
{
	int outputs = model.outputs();
	if(outputs < 2 || samples.inputs != model.inputs || samples.outputs != outputs)
		return -1;
	std::vector<std::vector<float> > values;
	std::vector<float> input;
	int right = 0, labeled = 0;
	for(int first = 0; first < samples.size(); first += ACCURACY_CHUNK)
	{
		int count = std::min(ACCURACY_CHUNK, samples.size() - first);
		input.resize((size_t)count * model.inputs);
		for(int s = 0; s < count; s++)
			samples.input(first + s, &input[(size_t)s * model.inputs]);
		model.forward(input.data(), count, values);
		for(int s = 0; s < count; s++)
		{
			int label = samples.label(first + s);
			if(label < 0)
				continue;
			labeled++;
			if(recognized(&values.back()[(size_t)s * outputs], outputs) == label)
				right++;
		}
	}
	return labeled > 0 ? (double)right / labeled : -1;
}

Trainer::Parameters Trainer::parameters(const Model& model, float value)
{
	Parameters result;
	for(const Model::Layer& layer : model.layers)
	{
		result.weights.push_back(std::vector<float>(layer.weights.size(), value));
		result.bias.push_back(std::vector<float>(layer.bias.size(), value));
	}
	return result;
}

void Trainer::clear(Parameters& parameters)
{
	for(size_t l = 0; l < parameters.weights.size(); l++)
	{
		std::fill(parameters.weights[l].begin(), parameters.weights[l].end(), 0.0f);
		std::fill(parameters.bias[l].begin(), parameters.bias[l].end(), 0.0f);
	}
}

bool Trainer::differs(const Model& model, const Model& other)
{
	for(size_t l = 0; l < model.layers.size(); l++)
	{
		if(model.layers[l].weights != other.layers[l].weights ||
			model.layers[l].bias != other.layers[l].bias)
			return true;
	}
	return false;
}

bool Trainer::isFixed(int layer, int neuron)
{
	return layer < (int)fixedBiases.size() && neuron < (int)fixedBiases[layer].size() &&
		fixedBiases[layer][neuron];
}

Trainer::Evaluation Trainer::evaluate(const Model& model, const float* input, const float* target,
	int count, Parameters& gradient, float scale)
{
	Evaluation evaluation = { 0, 0, true, 0, 0 };
	model.forward(input, count, values);
	int last = (int)model.layers.size() - 1;
	deltas.resize(model.layers.size());
	// erro das saídas: a diferença para o alvo vezes a derivada
	const Model::Layer& output = model.layers[last];
	int outputs = output.size;
	deltas[last].resize((size_t)count * outputs);
	for(int s = 0; s < count; s++)
	{
		const float* value = &values[last][(size_t)s * outputs];
		const float* goal = target + (size_t)s * outputs;
		float* delta = &deltas[last][(size_t)s * outputs];
		int label = -1;
		for(int o = 0; o < outputs; o++)
		{
			float difference = goal[o] - value[o];
			evaluation.squared += difference * difference;
			evaluation.absolute += fabsf(difference);
			evaluation.onTarget = evaluation.onTarget && fabsf(difference) < Model::Tolerance;
			delta[o] = difference * Model::derivative(output.function, output.beta, value[o]);
			label = goal[o] == 0 ? label : label == -1 ? o : -2;
		}
		if(outputs > 1 && label >= 0)
		{
			evaluation.labeled++;
			if(recognized(value, outputs) == label)
				evaluation.right++;
		}
	}
	// das saídas para as entradas: o peso anda no sentido do erro do neurônio
	// vezes o valor da entrada, o bias é o peso de uma entrada que vale 1
	for(int l = last; l >= 0; l--)
	{
		const Model::Layer& layer = model.layers[l];
		int size = layer.size, k = model.inputCount(l);
		const float* in = l == 0 ? input : values[l - 1].data();
		float* weights = gradient.weights[l].data();
		float* bias = gradient.bias[l].data();
		for(int s = 0; s < count; s++)
		{
			const float* delta = &deltas[l][(size_t)s * size];
			const float* x = in + (size_t)s * k;
			for(int j = 0; j < size; j++)
				bias[j] += delta[j] * scale;
			for(int i = 0; i < k; i++)
			{
				float value = x[i] * scale;
				if(value == 0)
					continue;
				float* weight = weights + (size_t)i * size;
				for(int j = 0; j < size; j++)
					weight[j] += value * delta[j];
			}
		}
		if(l == 0)
			break;
		// erro de cada neurônio da camada anterior: a soma dos erros que ele
		// causou, pelos pesos, vezes a derivada
		const Model::Layer& previous = model.layers[l - 1];
		deltas[l - 1].resize((size_t)count * k);
		for(int s = 0; s < count; s++)
		{
			const float* delta = &deltas[l][(size_t)s * size];
			const float* value = &values[l - 1][(size_t)s * k];
			float* result = &deltas[l - 1][(size_t)s * k];
			for(int i = 0; i < k; i++)
			{
				float derivative = Model::derivative(previous.function, previous.beta, value[i]);
				float sum = 0;
				if(derivative != 0)
				{
					const float* weight = layer.weights.data() + (size_t)i * size;
					for(int j = 0; j < size; j++)
						sum += weight[j] * delta[j];
				}
				result[i] = sum * derivative;
			}
		}
	}
	return evaluation;
}

Model Trainer::gradient(const Model& model, const Dataset& samples)
{
	std::vector<float> input, target;
	for(int s = 0; s < samples.size(); s++)
	{
		std::vector<float> values = samples.input(s);
		input.insert(input.end(), values.begin(), values.end());
		target.insert(target.end(), samples.target(s), samples.target(s) + samples.outputs);
	}
	Parameters sum = parameters(model, 0);
	evaluate(model, input.data(), target.data(), samples.size(), sum, 1);
	Model result = model;
	for(size_t l = 0; l < model.layers.size(); l++)
	{
		result.layers[l].weights = sum.weights[l];
		result.layers[l].bias = sum.bias[l];
	}
	return result;
}

Trainer::Result Trainer::train(Model& model, const Dataset& samples, const Dataset* test)
{
	Result result = { Trained, byEpochs(samples.size()), 0, 0, 0, -1, -1, samples.size(), false };
	if(model.layers.empty())
		result.status = NoOutput;
	else if(samples.inputs != model.inputs || samples.outputs != model.outputs())
		result.status = Mismatch;
	else if(samples.empty())
		result.status = NoSamples;
	else if(!model.isFinite())
		result.status = Invalid;
	else if(result.epochs)
		return sgd(model, samples, test);
	else
		return rprop(model, samples);
	return result;
}

/**
 * Um passo do Rprop em count valores, retorna o maior passo
 */
static float rpropStep(float* values, const float* gradient, float* step, float* last, int count,
	const std::vector<char>* fixed)
{
	float largest = 0;
	for(int i = 0; i < count; i++)
	{
		if(gradient[i] == 0 || (fixed != nullptr && (*fixed)[i]))
		{
			last[i] = 0;
			continue;
		}
		float direction = gradient[i] * last[i];
		if(direction > 0)
			step[i] = std::min(step[i] * 1.2f, Trainer::MaxStep);
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
	return largest;
}

Trainer::Result Trainer::rprop(Model& model, const Dataset& samples)
{
	Result result = { Trained, false, 0, 0, 0, -1, -1, samples.size(), false };
	int count = samples.size();
	std::vector<float> input((size_t)count * model.inputs), target;
	for(int s = 0; s < count; s++)
	{
		samples.input(s, &input[(size_t)s * model.inputs]);
		target.insert(target.end(), samples.target(s), samples.target(s) + samples.outputs);
	}
	std::vector<std::vector<char> > fixed;
	for(size_t l = 0; l < model.layers.size(); l++)
	{
		fixed.push_back(std::vector<char>(model.layers[l].size, 0));
		for(int j = 0; j < model.layers[l].size; j++)
			fixed[l][j] = isFixed((int)l, j);
	}
	Model start = model, best = model;
	Parameters gradient = parameters(model, 0), step = parameters(model, InitialStep);
	Parameters last = parameters(model, 0);
	Evaluation evaluation = evaluate(model, input.data(), target.data(), count, gradient, 1);
	result.errorBefore = result.errorAfter = evaluation.absolute;
	if(!std::isfinite(evaluation.squared))
	{
		result.status = Invalid;
		return result;
	}
	Evaluation lowest = evaluation;
	bool stalled = false, stopped = false;
	while(!evaluation.onTarget && result.steps < MaxIterations && std::isfinite(evaluation.squared))
	{
		if(stop)
		{
			stopped = true;
			break;
		}
		float largest = 0;
		for(size_t l = 0; l < model.layers.size(); l++)
		{
			Model::Layer& layer = model.layers[l];
			largest = std::max(largest, rpropStep(layer.weights.data(), gradient.weights[l].data(),
				step.weights[l].data(), last.weights[l].data(), (int)layer.weights.size(), nullptr));
			largest = std::max(largest, rpropStep(layer.bias.data(), gradient.bias[l].data(),
				step.bias[l].data(), last.bias[l].data(), layer.size, &fixed[l]));
		}
		if(largest < MinStep)
		{
			stalled = true;
			break;
		}
		result.steps++;
		clear(gradient);
		evaluation = evaluate(model, input.data(), target.data(), count, gradient, 1);
		if(evaluation.squared < lowest.squared)
		{
			lowest = evaluation;
			best = model;
		}
		if(progress)
			progress(Progress{ false, result.steps, MaxIterations, evaluation.absolute,
				evaluation.labeled > 0 ? (double)evaluation.right / evaluation.labeled : -1, -1 });
	}
	// fica com os pesos que acertam todas as amostras, mesmo que outros
	// tenham um erro menor sem acertar; nan também volta para os melhores
	if(!evaluation.onTarget && !(evaluation.squared <= lowest.squared))
	{
		model = best;
		evaluation = lowest;
	}
	result.errorAfter = evaluation.absolute;
	result.accuracy = evaluation.labeled > 0 ? (double)evaluation.right / evaluation.labeled : -1;
	result.changed = differs(model, start);
	if(evaluation.onTarget)
		result.status = result.steps == 0 ? OnTarget : Trained;
	else if(stopped)
		result.status = Stopped;
	else if(stalled)
		result.status = Stalled;
	else
		result.status = NotConverged;
	return result;
}

Trainer::Result Trainer::sgd(Model& model, const Dataset& samples, const Dataset* test)
{
	Result result = { Finished, true, 0, -1, -1, -1, -1, samples.size(), false };
	int count = samples.size(), outputs = model.outputs();
	if(test != nullptr && (test->inputs != model.inputs || test->outputs != outputs))
		test = nullptr;
	std::mt19937 random(seed);
	std::vector<int> order(count);
	std::iota(order.begin(), order.end(), 0);
	std::vector<float> input((size_t)batch * model.inputs), target((size_t)batch * outputs);
	Parameters gradient = parameters(model, 0);
	Model initial = model;
	for(int epoch = 1; epoch <= epochs && result.status == Finished; epoch++)
	{
		std::shuffle(order.begin(), order.end(), random);
		// a época que diverge volta aos pesos do começo dela
		Model start = model;
		double error = 0;
		int right = 0, labeled = 0, done = 0;
		for(int first = 0; first < count; first += batch)
		{
			if(stop)
			{
				result.status = Stopped;
				break;
			}
			int size = std::min(batch, count - first);
			for(int s = 0; s < size; s++)
			{
				samples.input(order[first + s], &input[(size_t)s * model.inputs]);
				std::copy(samples.target(order[first + s]), samples.target(order[first + s]) + outputs,
					&target[(size_t)s * outputs]);
			}
			clear(gradient);
			// o gradiente é a média do lote
			Evaluation evaluation = evaluate(model, input.data(), target.data(), size, gradient,
				1.0f / size);
			error += evaluation.absolute;
			right += evaluation.right;
			labeled += evaluation.labeled;
			done += size;
			for(size_t l = 0; l < model.layers.size(); l++)
			{
				Model::Layer& layer = model.layers[l];
				for(size_t i = 0; i < layer.weights.size(); i++)
					layer.weights[i] += rate * gradient.weights[l][i];
				for(int j = 0; j < layer.size; j++)
				{
					if(!isFixed((int)l, j))
						layer.bias[j] += rate * gradient.bias[l][j];
				}
			}
		}
		if(!std::isfinite(error) || !model.isFinite())
		{
			model = start;
			result.status = Invalid;
			break;
		}
		if(done < count)
			break;
		result.steps = epoch;
		result.errorAfter = error / count;
		result.accuracy = labeled > 0 ? (double)right / labeled : -1;
		if(test != nullptr)
			result.testAccuracy = accuracy(model, *test);
		if(progress)
			progress(Progress{ true, epoch, epochs, result.errorAfter, result.accuracy,
				result.testAccuracy });
	}
	result.changed = differs(model, initial);
	return result;
}
