// Baseado em renderers/raylib/clay_renderer_raylib.c do Clay (zlib)
#define CLAY_IMPLEMENTATION
#include "ClayRaylib.h"
#include "Resources.h"
#include "Shapes.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void* clayMemory = NULL;
// pixels da janela por unidade da interface
static float scale = 1;
// o raylib exige strings terminadas em '\0' e o Clay entrega fatias
static std::string textBuffer;

static const char* terminate(const char* chars, int32_t length)
{
	textBuffer.assign(chars, length);
	return textBuffer.c_str();
}

static Color toColor(Clay_Color color)
{
	Color result;
	result.r = (unsigned char)roundf(color.r);
	result.g = (unsigned char)roundf(color.g);
	result.b = (unsigned char)roundf(color.b);
	result.a = (unsigned char)roundf(color.a);
	return result;
}

/**
 * Texto medido com a fonte do tamanho desenhado, em unidades da interface
 */
static Vector2 measure(const char* text, uint16_t fontSize, uint16_t letterSpacing)
{
	float size = fontSize * scale;
	Vector2 measured = MeasureTextEx(getFont(size), text, size, letterSpacing * scale);
	return Vector2{ measured.x / scale, measured.y / scale };
}

static Clay_Dimensions measureText(Clay_StringSlice text, Clay_TextElementConfig* config,
	void* userData)
{
	Vector2 size = measure(terminate(text.chars, text.length), config->fontSize,
		config->letterSpacing);
	Clay_Dimensions dimensions = { size.x, size.y };
	return dimensions;
}

static void handleError(Clay_ErrorData error)
{
	fprintf(stderr, "Clay: %.*s\n", (int)error.errorText.length, error.errorText.chars);
}

void ClayRaylib_Initialize()
{
	uint32_t size = Clay_MinMemorySize();
	clayMemory = malloc(size);
	Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(size, clayMemory);
	Clay_Dimensions dimensions = ClayRaylib_GetLayoutDimensions();
	Clay_ErrorHandler errorHandler = { handleError, NULL };
	Clay_Initialize(arena, dimensions, errorHandler);
	Clay_SetMeasureTextFunction(measureText, NULL);
}

void ClayRaylib_Shutdown()
{
	free(clayMemory);
	clayMemory = NULL;
}

/**
 * Largura da borda na janela, em pixels inteiros para a linha ficar nítida
 */
static float borderWidth(uint16_t width)
{
	return width > 0 ? fmaxf(1, roundf(width * scale)) : 0;
}

static void drawBorder(Rectangle rect, const Clay_BorderRenderData& border)
{
	Color color = toColor(border.color);
	float left = borderWidth(border.width.left), right = borderWidth(border.width.right);
	float top = borderWidth(border.width.top), bottom = borderWidth(border.width.bottom);
	float radius = border.cornerRadius.topLeft * scale;
	bool uniform = left == right && left == top && left == bottom;
	if(uniform && left > 0)
	{
		if(radius > 0)
			shapes::strokeRoundedRect(rect, radius, left, color);
		else
			DrawRectangleLinesEx(rect, left, color);
		return;
	}
	if(left > 0)
		DrawRectangleRec(Rectangle{ rect.x, rect.y, left, rect.height }, color);
	if(right > 0)
		DrawRectangleRec(Rectangle{ rect.x + rect.width - right, rect.y, right, rect.height },
			color);
	if(top > 0)
		DrawRectangleRec(Rectangle{ rect.x, rect.y, rect.width, top }, color);
	if(bottom > 0)
		DrawRectangleRec(Rectangle{ rect.x, rect.y + rect.height - bottom, rect.width, bottom },
			color);
}

void ClayRaylib_Render(Clay_RenderCommandArray commands)
{
	for(int32_t i = 0; i < commands.length; i++)
	{
		Clay_RenderCommand* command = Clay_RenderCommandArray_Get(&commands, i);
		Clay_BoundingBox box = command->boundingBox;
		Rectangle rect = { roundf(box.x * scale), roundf(box.y * scale), roundf(box.width * scale),
			roundf(box.height * scale) };
		switch(command->commandType)
		{
		case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
		{
			Clay_RectangleRenderData& data = command->renderData.rectangle;
			if(data.cornerRadius.topLeft > 0)
				shapes::fillRoundedRect(rect, data.cornerRadius.topLeft * scale,
					toColor(data.backgroundColor));
			else
				DrawRectangleRec(rect, toColor(data.backgroundColor));
			break;
		}
		case CLAY_RENDER_COMMAND_TYPE_BORDER:
			drawBorder(rect, command->renderData.border);
			break;
		case CLAY_RENDER_COMMAND_TYPE_TEXT:
		{
			Clay_TextRenderData& data = command->renderData.text;
			float size = data.fontSize * scale;
			DrawTextEx(getFont(size),
				terminate(data.stringContents.chars, data.stringContents.length),
				Vector2{ rect.x, rect.y }, size, data.letterSpacing * scale,
				toColor(data.textColor));
			break;
		}
		case CLAY_RENDER_COMMAND_TYPE_IMAGE:
		{
			Clay_ImageRenderData& data = command->renderData.image;
			Texture2D texture = *(Texture2D*)data.imageData;
			Color tint = data.backgroundColor.a > 0 ? toColor(data.backgroundColor) : WHITE;
			DrawTexturePro(texture, Rectangle{ 0, 0, (float)texture.width, (float)texture.height },
				rect, Vector2{ 0, 0 }, 0, tint);
			break;
		}
		case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
			BeginScissorMode((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height);
			break;
		case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
			EndScissorMode();
			break;
		case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
		{
			CustomDraw* custom = (CustomDraw*)command->renderData.custom.customData;
			if(custom != NULL && custom->draw != NULL)
				custom->draw(custom->context, rect);
			break;
		}
		default:
			break;
		}
	}
}

void ClayRaylib_SetScale(float value)
{
	if(value == scale)
		return;
	scale = value;
	// o Clay guarda a largura medida de cada texto, que muda com a escala
	Clay_ResetMeasureTextCache();
}

float ClayRaylib_GetScale()
{
	return scale;
}

Clay_Dimensions ClayRaylib_GetLayoutDimensions()
{
	return Clay_Dimensions{ GetScreenWidth() / scale, GetScreenHeight() / scale };
}

Vector2 ClayRaylib_ToLayout(Vector2 point)
{
	return Vector2{ point.x / scale, point.y / scale };
}

float ClayRaylib_MeasureText(const std::string& text, uint16_t fontSize)
{
	return measure(text.c_str(), fontSize, 0).x;
}
