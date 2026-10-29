#include "Resources.h"
#include <map>
#include <math.h>
#include <stdexcept>
#include <vector>
// gerado pelo Makefile a partir de res/
#include "Roboto-Regular.ttf.h"

static std::map<int, Font> fonts;
static std::vector<int> codepoints;

float getRenderScale()
{
	int width = GetScreenWidth();
	return width > 0 ? (float)GetRenderWidth() / width : 1;
}

static Font loadFont(int pixels)
{
	if(codepoints.empty())
	{
		// ASCII + Latin-1, cobre os acentos do português
		for(int c = 32; c < 127; c++)
			codepoints.push_back(c);
		for(int c = 160; c < 256; c++)
			codepoints.push_back(c);
	}
	Font font = LoadFontFromMemory(".ttf", Roboto_Regular_ttf, Roboto_Regular_ttf_len, pixels,
		codepoints.data(), (int)codepoints.size());
	if(!IsFontValid(font))
		throw std::runtime_error("Unable to load the interface font.");
	SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
	return font;
}

void loadResources()
{
	getFont(13);
}

void unloadResources()
{
	for(auto& item : fonts)
		UnloadFont(item.second);
	fonts.clear();
}

const Font& getFont(float size)
{
	int pixels = (int)roundf(size * getRenderScale());
	if(pixels < 1)
		pixels = 1;
	// os tamanhos grandes vêm do zoom da área de edição: um a cada 4 pixels,
	// para não carregar uma fonte a cada passo do zoom
	if(pixels > 32)
		pixels = (pixels + 2) / 4 * 4;
	auto it = fonts.find(pixels);
	if(it != fonts.end())
		return it->second;
	return fonts[pixels] = loadFont(pixels);
}
