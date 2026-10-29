#include "Resources.h"
#include <stdexcept>
#include <vector>
// gerado pelo Makefile a partir de res/
#include "Roboto-Regular.ttf.h"

// tamanho em que a fonte é rasterizada, reduzida com mipmaps ao desenhar
#define FONT_BASE_SIZE 32

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
