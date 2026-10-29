// Baseado em renderers/raylib/clay_renderer_raylib.c do Clay (zlib)
#define CLAY_IMPLEMENTATION
#include "ClayRaylib.h"
#include "Shapes.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void* clayMemory = NULL;
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

static Clay_Dimensions measureText(Clay_StringSlice text, Clay_TextElementConfig* config,
	void* userData)
{
	Font* fonts = (Font*)userData;
	Vector2 size = MeasureTextEx(fonts[config->fontId], terminate(text.chars, text.length),
		config->fontSize, config->letterSpacing);
	Clay_Dimensions dimensions = { size.x, size.y };
	return dimensions;
}

static void handleError(Clay_ErrorData error)
{
	fprintf(stderr, "Clay: %.*s\n", (int)error.errorText.length, error.errorText.chars);
}

void ClayRaylib_Initialize(Font* fonts)
{
	uint32_t size = Clay_MinMemorySize();
	clayMemory = malloc(size);
	Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(size, clayMemory);
	Clay_Dimensions dimensions = { (float)GetScreenWidth(), (float)GetScreenHeight() };
	Clay_ErrorHandler errorHandler = { handleError, NULL };
	Clay_Initialize(arena, dimensions, errorHandler);
	Clay_SetMeasureTextFunction(measureText, fonts);
}

void ClayRaylib_Shutdown()
{
	free(clayMemory);
	clayMemory = NULL;
}

static void drawBorder(Rectangle rect, const Clay_BorderRenderData& border)
{
	Color color = toColor(border.color);
	const Clay_BorderWidth& width = border.width;
	float radius = border.cornerRadius.topLeft;
	bool uniform = width.left == width.right && width.left == width.top &&
		width.left == width.bottom;
	if(uniform && width.left > 0)
	{
		if(radius > 0)
			shapes::strokeRoundedRect(rect, radius, width.left, color);
		else
			DrawRectangleLinesEx(rect, width.left, color);
		return;
	}
	if(width.left > 0)
		DrawRectangleRec(Rectangle{ rect.x, rect.y, (float)width.left, rect.height }, color);
	if(width.right > 0)
		DrawRectangleRec(Rectangle{ rect.x + rect.width - width.right, rect.y,
			(float)width.right, rect.height }, color);
	if(width.top > 0)
		DrawRectangleRec(Rectangle{ rect.x, rect.y, rect.width, (float)width.top }, color);
	if(width.bottom > 0)
		DrawRectangleRec(Rectangle{ rect.x, rect.y + rect.height - width.bottom,
			rect.width, (float)width.bottom }, color);
}

void ClayRaylib_Render(Clay_RenderCommandArray commands, Font* fonts)
{
	for(int32_t i = 0; i < commands.length; i++)
	{
		Clay_RenderCommand* command = Clay_RenderCommandArray_Get(&commands, i);
		Clay_BoundingBox box = command->boundingBox;
		Rectangle rect = { roundf(box.x), roundf(box.y), roundf(box.width), roundf(box.height) };
		switch(command->commandType)
		{
		case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
		{
			Clay_RectangleRenderData& data = command->renderData.rectangle;
			if(data.cornerRadius.topLeft > 0)
				shapes::fillRoundedRect(rect, data.cornerRadius.topLeft,
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
			DrawTextEx(fonts[data.fontId],
				terminate(data.stringContents.chars, data.stringContents.length),
				Vector2{ rect.x, rect.y }, data.fontSize, data.letterSpacing,
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
