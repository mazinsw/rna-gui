#include "Dataset.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
// formato IDX: número mágico, quantidade e dimensões em u32 big-endian
#define IDX_IMAGES 0x00000803
#define IDX_LABELS 0x00000801
#define IDX_TRAIN "train-images-idx3-ubyte"

/**
 * Caminho em UTF-8, para as mensagens
 */
static std::string utf8(const std::filesystem::path& path)
{
	std::u8string str = path.u8string();
	return std::string(str.begin(), str.end());
}

/**
 * Número com ponto decimal e só os dígitos necessários para ser lido igual
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

Dataset::Dataset(int inputs, int outputs): inputs(inputs), outputs(outputs)
{
}

int Dataset::size() const
{
	return outputs > 0 ? (int)(targets.size() / outputs) : 0;
}

bool Dataset::empty() const
{
	return targets.empty();
}

void Dataset::clear()
{
	values.clear();
	bytes.clear();
	targets.clear();
}

void Dataset::add(const std::vector<float>& input, const std::vector<float>& target)
{
	// as imagens em bytes passam a valores para receber os da amostra
	if(!bytes.empty())
	{
		values.resize(bytes.size());
		for(size_t i = 0; i < bytes.size(); i++)
			values[i] = bytes[i] / 255.0f;
		bytes.clear();
	}
	values.insert(values.end(), input.begin(), input.begin() + inputs);
	targets.insert(targets.end(), target.begin(), target.begin() + outputs);
}

void Dataset::addImage(const std::vector<float>& pixels, const std::vector<float>& target)
{
	if(!values.empty())
		return add(pixels, target);
	for(int i = 0; i < inputs; i++)
		bytes.push_back((unsigned char)lroundf(fminf(fmaxf(pixels[i], 0), 1) * 255));
	targets.insert(targets.end(), target.begin(), target.begin() + outputs);
}

bool Dataset::remember(const std::vector<float>& input, const std::vector<float>& target)
{
	for(int s = 0; s < size(); s++)
	{
		if(this->input(s) != input)
			continue;
		float* old = &targets[(size_t)s * outputs];
		if(std::equal(target.begin(), target.end(), old))
			return false;
		std::copy(target.begin(), target.end(), old);
		return true;
	}
	add(input, target);
	return true;
}

void Dataset::input(int sample, float* out) const
{
	size_t start = (size_t)sample * inputs;
	if(bytes.empty())
	{
		std::copy(values.begin() + start, values.begin() + start + inputs, out);
		return;
	}
	for(int i = 0; i < inputs; i++)
		out[i] = bytes[start + i] / 255.0f;
}

std::vector<float> Dataset::input(int sample) const
{
	std::vector<float> result(inputs);
	input(sample, result.data());
	return result;
}

const float* Dataset::target(int sample) const
{
	return &targets[(size_t)sample * outputs];
}

int Dataset::label(int sample) const
{
	const float* target = this->target(sample);
	int label = -1;
	for(int o = 0; o < outputs; o++)
	{
		if(target[o] == 0)
			continue;
		if(label >= 0)
			return -1;
		label = o;
	}
	return label;
}

std::filesystem::path Dataset::samplesPath(const std::filesystem::path& network)
{
	std::filesystem::path path = network;
	path.replace_extension(".amostras.txt");
	return path;
}

void Dataset::saveSamples(const std::filesystem::path& network) const
{
	std::filesystem::path path = samplesPath(network);
	if(empty())
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
	file << "amostras " << size() << "\n";
	std::vector<float> values(inputs);
	for(int s = 0; s < size(); s++)
	{
		input(s, values.data());
		for(int i = 0; i < inputs; i++)
			file << (i > 0 ? " " : "") << format(values[i]);
		file << "\nsaidas";
		for(int o = 0; o < outputs; o++)
			file << " " << format(target(s)[o]);
		file << "\n";
	}
	file.close();
	if(!file)
		throw std::runtime_error("Unable to write " + utf8(path) + ".");
}

Dataset Dataset::loadSamples(const std::filesystem::path& network, int inputs, int outputs)
{
	Dataset samples(inputs, outputs);
	std::filesystem::path path = samplesPath(network);
	std::error_code error;
	if(!std::filesystem::exists(path, error))
		return samples;
	std::ifstream file(path, std::ios::binary);
	if(!file)
		throw std::runtime_error("Unable to open " + utf8(path) + ".");
	std::string word;
	long count;
	if(!(file >> word) || word != "amostras" || !(file >> count) || count < 0)
		throw std::runtime_error("Expected \"amostras\" and the number of samples.");
	std::vector<float> input, target;
	for(long n = 1; n <= count; n++)
	{
		std::runtime_error mismatch("Sample " + std::to_string(n) + " does not have the " +
			std::to_string(inputs) + " inputs and " + std::to_string(outputs) +
			" outputs of the network.");
		// lê os valores com ponto decimal, como os arquivos do console
		auto readValues = [&](int size, std::vector<float>& values)
		{
			values.clear();
			for(int i = 0; i < size; i++)
			{
				char *end;
				if(!(file >> word))
					throw mismatch;
				double value = strtod(word.c_str(), &end);
				if(*end != '\0' || end == word.c_str() || !std::isfinite(value))
					throw mismatch;
				values.push_back((float)value);
			}
		};
		readValues(inputs, input);
		if(!(file >> word) || word != "saidas")
			throw mismatch;
		readValues(outputs, target);
		samples.add(input, target);
	}
	if(file >> word)
		throw std::runtime_error("More values than the " + std::to_string(count) +
			" samples declared.");
	return samples;
}

static uint32_t readBigEndian(std::ifstream& file, const std::filesystem::path& path)
{
	unsigned char bytes[4];
	file.read((char*)bytes, sizeof(bytes));
	if(!file)
		throw std::runtime_error("Unexpected end of " + utf8(path.filename()) + ".");
	return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 | (uint32_t)bytes[2] << 8 | bytes[3];
}

/**
 * Arquivo de imagens IDX: o próprio ou o de treino dentro da pasta
 */
static std::filesystem::path imagesFile(const std::filesystem::path& path)
{
	std::error_code error;
	if(std::filesystem::is_directory(path, error))
		return path / IDX_TRAIN;
	return path;
}

/**
 * Troca no nome do arquivo cada trecho from por to
 */
static std::filesystem::path rename(const std::filesystem::path& path, const std::string& from,
	const std::string& to)
{
	std::string name = utf8(path.filename());
	for(size_t pos = name.find(from); pos != std::string::npos; pos = name.find(from, pos + to.size()))
		name.replace(pos, from.size(), to);
	std::u8string text(name.begin(), name.end());
	return path.parent_path() / std::filesystem::path(text);
}

bool Dataset::isIdx(const std::filesystem::path& path)
{
	std::error_code error;
	std::filesystem::path images = imagesFile(path);
	return utf8(images.filename()).find("idx3") != std::string::npos &&
		std::filesystem::is_regular_file(images, error);
}

Dataset Dataset::loadIdx(const std::filesystem::path& path)
{
	std::filesystem::path imagesPath = imagesFile(path);
	std::filesystem::path labelsPath = rename(rename(imagesPath, "images", "labels"), "idx3",
		"idx1");
	std::ifstream images(imagesPath, std::ios::binary), labels(labelsPath, std::ios::binary);
	if(!images)
		throw std::runtime_error("Unable to open " + utf8(imagesPath) + ".");
	if(!labels)
		throw std::runtime_error("Unable to open " + utf8(labelsPath) + ".");
	if(readBigEndian(images, imagesPath) != IDX_IMAGES)
		throw std::runtime_error(utf8(imagesPath.filename()) + " is not an IDX image file.");
	if(readBigEndian(labels, labelsPath) != IDX_LABELS)
		throw std::runtime_error(utf8(labelsPath.filename()) + " is not an IDX label file.");
	uint32_t count = readBigEndian(images, imagesPath);
	uint32_t rows = readBigEndian(images, imagesPath);
	uint32_t columns = readBigEndian(images, imagesPath);
	if(readBigEndian(labels, labelsPath) != count)
		throw std::runtime_error(utf8(labelsPath.filename()) + " does not have one label per "
			"image.");
	if(rows == 0 || columns == 0 || rows > 4096 || columns > 4096 || count > 100000000)
		throw std::runtime_error(utf8(imagesPath.filename()) + " has an invalid size.");
	Dataset dataset((int)(rows * columns), 0);
	std::vector<unsigned char> classes(count);
	labels.read((char*)classes.data(), count);
	if(!labels)
		throw std::runtime_error("Unexpected end of " + utf8(labelsPath.filename()) + ".");
	dataset.bytes.resize((size_t)count * dataset.inputs);
	images.read((char*)dataset.bytes.data(), dataset.bytes.size());
	if(!images)
		throw std::runtime_error("Unexpected end of " + utf8(imagesPath.filename()) + ".");
	for(unsigned char label : classes)
		dataset.outputs = std::max(dataset.outputs, label + 1);
	dataset.targets.assign((size_t)count * dataset.outputs, 0);
	for(uint32_t s = 0; s < count; s++)
		dataset.targets[(size_t)s * dataset.outputs + classes[s]] = 1;
	return dataset;
}

std::filesystem::path Dataset::testPath(const std::filesystem::path& path)
{
	std::filesystem::path images = imagesFile(path);
	std::string name = utf8(images.filename());
	if(name.compare(0, 5, "train") != 0)
		return std::filesystem::path();
	std::filesystem::path test = rename(images, "train", "t10k");
	std::error_code error;
	return std::filesystem::is_regular_file(test, error) ? test : std::filesystem::path();
}
