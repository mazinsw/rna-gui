#include "Draw.h"
#include "Icons.h"
#include "Number.h"
#include "Resources.h"
#include "Shapes.h"
#include "Widgets.h"
#include "../training/Network.h"
#include <cmath>

static float viewZoom = 1;
static Vector2 viewOrigin = { 0, 0 };

void draw::setView(float zoom, Vector2 origin)
{
	viewZoom = zoom;
	viewOrigin = origin;
}

Vector2 draw::measureText(const std::string& text, float fontSize)
{
	return MeasureTextEx(getFont(fontSize * viewZoom), text.c_str(), fontSize, 0);
}

/**
 * Coordenada da rede no pixel da tela mais próximo
 */
static float toPixel(float value, float origin)
{
	float scale = getRenderScale();
	return (roundf((value * viewZoom + origin) * scale) / scale - origin) / viewZoom;
}

void draw::text(const std::string& text, Vector2 position, float fontSize, Color color)
{
	// no pixel da tela, para não borrar o texto
	DrawTextEx(getFont(fontSize * viewZoom), text.c_str(), Vector2{
		toPixel(position.x, viewOrigin.x), toPixel(position.y, viewOrigin.y) }, fontSize, 0,
		color);
}

void draw::textCentered(const std::string& text, Vector2 center, float fontSize, Color color)
{
	Vector2 size = measureText(text, fontSize);
	draw::text(text, Vector2{ center.x - size.x / 2, center.y - size.y / 2 }, fontSize, color);
}

void draw::pill(Rectangle box, Color fill, Color border, float borderWidth)
{
	shapes::fillRoundedRect(box, box.height / 2, fill);
	shapes::strokeRoundedRect(box, box.height / 2, borderWidth, border);
}

Rectangle draw::pillBox(Vector2 center, const std::string& text, float fontSize, float height)
{
	float width = measureText(text, fontSize).x + 16;
	return Rectangle{ center.x - width / 2, center.y - height / 2, width, height };
}

Color draw::influenceColor(float influence)
{
	static const struct
	{
		float position;
		Color color;
	} stops[] = {
		{ 0, { 203, 213, 225, 255 } },
		{ 0.35f, { 251, 191, 36, 255 } },
		{ 0.7f, { 249, 115, 22, 255 } },
		{ 1, { 220, 38, 38, 255 } },
	};
	for(int i = 1; i < 4; i++)
	{
		if(influence > stops[i].position && i < 3)
			continue;
		float t = (influence - stops[i - 1].position) / (stops[i].position - stops[i - 1].position);
		t = t < 0 ? 0 : t > 1 ? 1 : t;
		const Color& a = stops[i - 1].color;
		const Color& b = stops[i].color;
		return Color{ (unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
			(unsigned char)(a.b + (b.b - a.b) * t), 255 };
	}
	return stops[3].color;
}

void draw::input(Vector2 center, float value, bool running, bool selected)
{
	Rectangle box = { center.x - InputHalf, center.y - InputHalf, InputHalf * 2, InputHalf * 2 };
	if(selected)
		shapes::strokeRoundedRect(Rectangle{ box.x - 5, box.y - 5, box.width + 10,
			box.height + 10 }, 14, 4, ui::toColor(ui::SelectionRingColor));
	Color fill = ui::toColor(ui::AccentLightColor);
	Color border = ui::toColor(ui::AccentColor);
	Color text = ui::toColor(ui::AccentDarkColor);
	if(running && value > 0 && value < 1)
	{
		// pixel de uma imagem: a cor acompanha a intensidade
		fill = ColorLerp(WHITE, ui::toColor(ui::AccentColor), value);
		text = value >= 0.5f ? WHITE : ui::toColor(ui::AccentDarkColor);
	}
	else if(running && value != 0)
	{
		fill = ui::toColor(ui::AccentColor);
		text = WHITE;
	}
	else if(running)
		fill = WHITE;
	shapes::fillRoundedRect(box, 9, fill);
	shapes::strokeRoundedRect(box, 9, 2, border);
	if(running)
		textCentered(number::format(value, 2), center, 16, text);
	else
		icons::draw(icons::Arrow, Rectangle{ center.x - 8, center.y - 8, 16, 16 }, border);
}

void draw::neuron(Vector2 center, int function, bool selected)
{
	Color ink = ui::toColor(ui::NodeColor);
	shapes::fillCircle(Vector2{ center.x, center.y + 2 }, NeuronRadius + 1, Color{ 15, 23, 42, 30 });
	if(selected)
		shapes::strokeCircle(center, NeuronRadius + 3, 4, ui::toColor(ui::SelectionRingColor));
	shapes::fillCircle(center, NeuronRadius, WHITE);
	shapes::strokeCircle(center, NeuronRadius - 1, 2, ink);
	icons::drawFunction(function, Rectangle{ center.x - 15, center.y - 10, 30, 20 }, ink);
}

void draw::neuronValue(Vector2 center, float value)
{
	std::string text = "y = " + number::format(value, 2);
	Vector2 middle = { center.x, center.y + 45 };
	Rectangle box = pillBox(middle, text, ui::LabelFontSize, 20);
	Color dark = ui::toColor(std::isfinite(value) ? ui::InkColor : ui::DangerColor);
	pill(box, dark, dark);
	textCentered(text, middle, ui::LabelFontSize, WHITE);
}

Rectangle draw::label(Vector2 center, const std::string& prefix, float value, bool changed,
	bool selected)
{
	std::string text = number::format(value, 3);
	float size = ui::LabelFontSize;
	float prefixWidth = prefix.empty() ? 0 : measureText(prefix, size).x;
	float width = prefixWidth + measureText(text, size).x + 16;
	Rectangle box = { center.x - width / 2, center.y - 11, width, 22 };
	if(changed)
		pill(box, ui::toColor(ui::ChangedColor), ui::toColor(ui::ChangedBorderColor));
	else if(selected)
		pill(box, ui::toColor(ui::AccentLightColor), ui::toColor(ui::AccentColor), 1.5f);
	else
		pill(box, WHITE, ui::toColor(ui::PillBorderColor));
	float textY = box.y + (box.height - measureText(text, size).y) / 2;
	if(!prefix.empty())
		draw::text(prefix, Vector2{ box.x + 8, textY }, size, ui::toColor(ui::MutedColor));
	draw::text(text, Vector2{ box.x + 8 + prefixWidth, textY }, size, ui::toColor(ui::InkColor));
	return box;
}

Rectangle draw::output(Vector2 center, float value, float target, bool running, bool selected)
{
	Color color = ui::toColor(ui::NodeColor);
	if(running && !std::isfinite(value))
		color = ui::toColor(ui::DangerColor);
	else if(running)
		color = ui::toColor(fabsf(target - value) < Network::Tolerance ? ui::SuccessColor :
			ui::WarningColor);
	if(selected)
		shapes::strokeCircle(center, OutputRadius + 3, 4, ui::toColor(ui::SelectionRingColor));
	shapes::fillCircle(center, OutputRadius, WHITE);
	shapes::strokeCircle(center, OutputRadius - 1.5f, 3, color);
	shapes::fillCircle(center, 6, color);
	if(!running)
		return Rectangle{ 0, 0, 0, 0 };
	// abaixo da saída: o valor calculado e o alvo, que alterna ao clicar
	std::string valueText = "saída " + number::format(value, 2);
	std::string goal = "alvo " + number::format(target, 2);
	float size = ui::LabelFontSize;
	float valueWidth = measureText(valueText, size).x + 18;
	float goalWidth = measureText(goal, size).x + 18;
	float x = center.x - (valueWidth + 6 + goalWidth) / 2;
	float y = center.y + 26;
	Rectangle valueBox = { x, y, valueWidth, 24 };
	pill(valueBox, WHITE, color);
	textCentered(valueText, Vector2{ valueBox.x + valueBox.width / 2, y + 12 }, size, color);
	Rectangle targetBox = { x + valueWidth + 6, y, goalWidth, 24 };
	if(target != 0)
		pill(targetBox, ui::toColor(ui::AccentColor), ui::toColor(ui::AccentColor));
	else
		pill(targetBox, WHITE, ui::toColor(ui::PillBorderColor));
	textCentered(goal, Vector2{ targetBox.x + targetBox.width / 2, y + 12 }, size,
		target != 0 ? WHITE : ui::toColor(ui::InkColor));
	return targetBox;
}
