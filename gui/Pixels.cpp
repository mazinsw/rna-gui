#include "Pixels.h"
#include "Platform.h"
#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <ctype.h>
#include <fstream>
#include <iterator>
#include <stdexcept>

// acima disto, depois de esticar o contraste, o pixel é traço
static const double InkThreshold = 0.3;

/**
 * Parte de cada um dos size pixels de um lado do quadrado na média de cada um
 * dos count pixels da imagem reduzida: weights[i][k] é a fração do pixel k
 * coberta pelo pixel i, dividida pela largura do pixel i
 */
static std::vector<std::vector<double> > areaWeights(int size, int count)
{
	std::vector<std::vector<double> > weights(count, std::vector<double>(size, 0));
	double scale = (double)size / count;
	for(int i = 0; i < count; i++)
	{
		double start = i * scale, end = (i + 1) * scale;
		for(int k = (int)start; k < size && k < end; k++)
			weights[i][k] = (std::min(end, k + 1.0) - std::max(start, (double)k)) / scale;
	}
	return weights;
}

std::vector<double> pixels::fromGray(const std::vector<float>& gray, int width, int height,
	int side)
{
	std::vector<double> result(side * side, 0);
	if(width <= 0 || height <= 0 || side <= 0)
		return result;
	// fundo claro pela média da borda: traço escuro no papel
	double border = 0;
	for(int x = 0; x < width; x++)
		border += gray[x] + gray[(height - 1) * width + x];
	for(int y = 0; y < height; y++)
		border += gray[y * width] + gray[y * width + width - 1];
	bool invert = border / (2 * (width + height)) > 0.5;
	std::vector<double> values(gray.begin(), gray.end());
	double low = 1, high = 0;
	for(double& value : values)
	{
		if(invert)
			value = 1 - value;
		low = std::min(low, value);
		high = std::max(high, value);
	}
	if(high - low < 1e-6)
		return result;
	int left = width, top = height, right = 0, bottom = 0;
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			double& value = values[y * width + x];
			value = (value - low) / (high - low);
			if(value <= InkThreshold)
				continue;
			left = std::min(left, x);
			right = std::max(right, x);
			top = std::min(top, y);
			bottom = std::max(bottom, y);
		}
	}
	// quadrado centrado no traço, o que fica fora da imagem é fundo
	int w = right - left + 1, h = bottom - top + 1, size = std::max(w, h);
	int x0 = left - (size - w) / 2, y0 = top - (size - h) / 2;
	int margin = (int)lround(side / 7.0), inner = side - 2 * margin;
	std::vector<std::vector<double> > weights = areaWeights(size, inner);
	for(int i = 0; i < inner; i++)
	{
		for(int j = 0; j < inner; j++)
		{
			double sum = 0;
			for(int k = 0; k < size; k++)
			{
				int y = y0 + k;
				if(weights[i][k] == 0 || y < 0 || y >= height)
					continue;
				for(int l = 0; l < size; l++)
				{
					int x = x0 + l;
					if(weights[j][l] == 0 || x < 0 || x >= width)
						continue;
					sum += weights[i][k] * weights[j][l] * values[y * width + x];
				}
			}
			// 3 casas bastam à rede e deixam curto o arquivo das amostras
			result[(i + margin) * side + j + margin] = round(sum * 1000) / 1000;
		}
	}
	return result;
}

/**
 * Extensão do arquivo em minúsculas, com o ponto
 */
static std::string extensionOf(const std::string& fileName)
{
	std::string extension = pathToUtf8(pathFromUtf8(fileName).extension());
	for(char& c : extension)
		c = (char)tolower((unsigned char)c);
	return extension;
}

std::vector<double> pixels::fromFile(const std::string& fileName, int side)
{
	// lido aqui, o raylib não abre caminhos UTF-8 no Windows
	std::ifstream file(pathFromUtf8(fileName), std::ios::binary);
	if(!file)
		throw std::runtime_error("Unable to open " + fileName + ".");
	std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	Image image = LoadImageFromMemory(extensionOf(fileName).c_str(), data.data(),
		(int)data.size());
	if(image.data == NULL)
		throw std::runtime_error("Unsupported image, use PNG.");
	Color *colors = LoadImageColors(image);
	std::vector<float> gray(image.width * image.height);
	for(size_t i = 0; i < gray.size(); i++)
	{
		const Color& c = colors[i];
		float alpha = c.a / 255.0f;
		float luma = (0.299f * c.r + 0.587f * c.g + 0.114f * c.b) / 255;
		gray[i] = luma * alpha + 1 - alpha;
	}
	int width = image.width, height = image.height;
	UnloadImageColors(colors);
	UnloadImage(image);
	return fromGray(gray, width, height, side);
}

Dataset pixels::fromFolder(const std::string& folder, int side, int classes)
{
	std::vector<std::filesystem::path> images;
	for(const auto& entry : std::filesystem::directory_iterator(pathFromUtf8(folder)))
	{
		std::string name = pathToUtf8(entry.path());
		if(isImage(name) && labelOf(name) >= 0)
			images.push_back(entry.path());
	}
	std::sort(images.begin(), images.end());
	if(images.empty())
		throw std::runtime_error("No image named after its digit, like 3_7.png, in " + folder +
			".");
	Dataset samples(side * side, classes);
	for(const std::filesystem::path& image : images)
	{
		std::string name = pathToUtf8(image);
		int label = labelOf(name);
		if(label >= classes)
			throw std::runtime_error("The digit of " + pathToUtf8(image.filename()) +
				" has no output, the network has " + std::to_string(classes) + ".");
		std::vector<double> values = fromFile(name, side);
		std::vector<float> target(classes, 0);
		target[label] = 1;
		samples.add(std::vector<float>(values.begin(), values.end()), target);
	}
	return samples;
}

bool pixels::isImage(const std::string& fileName)
{
	static const char *extensions[] = { ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tga",
		".qoi" };
	std::string extension = extensionOf(fileName);
	for(const char *image : extensions)
	{
		if(extension == image)
			return true;
	}
	return false;
}

int pixels::labelOf(const std::string& fileName)
{
	std::string name = pathToUtf8(pathFromUtf8(fileName).filename());
	if(name.empty() || !isdigit((unsigned char)name[0]) ||
		(name.size() > 1 && isdigit((unsigned char)name[1])))
		return -1;
	return name[0] - '0';
}
