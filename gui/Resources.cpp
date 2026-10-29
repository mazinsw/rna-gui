#include "Resources.h"
#include <stdexcept>
#include <string.h>
#include <string>
#include <vector>
// gerados pelo Makefile com xxd -i a partir de res/
#include "neuron.png.h"
#include "input.png.h"
#include "output.png.h"
#include "funcao_linear.png.h"
#include "funcao_degree.png.h"
#include "funcao_bipolar.png.h"
#include "funcao_logistic.png.h"
#include "funcao_hiperbolic.png.h"
#include "Roboto-Regular.ttf.h"

// tamanho em que a fonte é rasterizada, reduzida com mipmaps ao desenhar
#define FONT_BASE_SIZE 32

struct EmbeddedImage
{
	const char* name;
	const unsigned char* data;
	unsigned int size;
};

static const EmbeddedImage images[] = {
	{ "neuron", neuron_png, neuron_png_len },
	{ "input", input_png, input_png_len },
	{ "output", output_png, output_png_len },
	{ "funcao_linear", funcao_linear_png, funcao_linear_png_len },
	{ "funcao_degree", funcao_degree_png, funcao_degree_png_len },
	{ "funcao_bipolar", funcao_bipolar_png, funcao_bipolar_png_len },
	{ "funcao_logistic", funcao_logistic_png, funcao_logistic_png_len },
	{ "funcao_hiperbolic", funcao_hiperbolic_png, funcao_hiperbolic_png_len },
};

static Font fonts[1];

void loadResources()
{
	// ASCII + Latin-1, cobre os acentos do português
	std::vector<int> codepoints;
	for(int c = 32; c < 127; c++)
		codepoints.push_back(c);
	for(int c = 160; c < 256; c++)
		codepoints.push_back(c);
	fonts[0] = LoadFontFromMemory(".ttf", Roboto_Regular_ttf, Roboto_Regular_ttf_len,
		FONT_BASE_SIZE, codepoints.data(), (int)codepoints.size());
	if(!IsFontValid(fonts[0]))
		throw std::runtime_error("Unable to load the interface font.");
	GenTextureMipmaps(&fonts[0].texture);
	SetTextureFilter(fonts[0].texture, TEXTURE_FILTER_TRILINEAR);
}

void unloadResources()
{
	UnloadFont(fonts[0]);
}

Font* getFonts()
{
	return fonts;
}

const Font& getFont()
{
	return fonts[0];
}

Texture2D loadTextureResource(const char* name)
{
	for(const EmbeddedImage& image : images)
	{
		if(strcmp(image.name, name) != 0)
			continue;
		Image img = LoadImageFromMemory(".png", image.data, (int)image.size);
		Texture2D texture = LoadTextureFromImage(img);
		UnloadImage(img);
		if(!IsTextureValid(texture))
			break;
		SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
		return texture;
	}
	throw std::runtime_error(std::string("Unable to load the image ") + name + ".");
}
