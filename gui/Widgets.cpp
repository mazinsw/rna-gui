#include "Widgets.h"
#include "Input.h"
#include "Resources.h"
#include <math.h>
#include <string.h>

namespace ui
{
	// botão em que o mouse foi pressionado, o clique só vale se soltar nele
	static uint32_t pressedId = 0;

	void endFrame()
	{
		if(!input::mouseDown())
			pressedId = 0;
	}

	Clay_TextElementConfig* textConfig(uint16_t fontSize, Clay_Color color,
		Clay_TextElementConfigWrapMode wrapMode)
	{
		return CLAY_TEXT_CONFIG({ .textColor = color, .fontId = 0, .fontSize = fontSize,
			.wrapMode = wrapMode });
	}

	bool button(Clay_ElementId id, Clay_String text, bool enabled, float minWidth)
	{
		bool hovered = enabled && Clay_PointerOver(id);
		if(hovered && input::mousePressed())
			pressedId = id.id;
		bool pressed = hovered && pressedId == id.id && input::mouseDown();
		bool clicked = hovered && pressedId == id.id && input::mouseReleased();
		Clay_Color background = ButtonColor;
		if(pressed)
			background = ButtonPressedColor;
		else if(hovered)
			background = ButtonHoverColor;
		CLAY({ .id = id,
			.layout = { .sizing = { CLAY_SIZING_FIT(minWidth), CLAY_SIZING_FIXED(ButtonHeight) },
				.padding = { 12, 12, 0, 0 },
				.childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER } },
			.backgroundColor = background,
			.cornerRadius = CLAY_CORNER_RADIUS(5),
			.border = { .color = BorderColor, .width = CLAY_BORDER_OUTSIDE(1) } })
		{
			CLAY_TEXT(text, textConfig(FontSize, enabled ? TextColor : DisabledTextColor));
		}
		return clicked;
	}

	/**
	 * Estado do mouse sobre um controle: retorna true no clique e informa se
	 * está sob o mouse e pressionado
	 */
	static bool track(Clay_ElementId id, bool enabled, bool& hovered, bool& pressed)
	{
		hovered = enabled && Clay_PointerOver(id);
		if(hovered && input::mousePressed())
			pressedId = id.id;
		pressed = hovered && pressedId == id.id && input::mouseDown();
		return hovered && pressedId == id.id && input::mouseReleased();
	}

	bool toolButton(Clay_ElementId id, icons::Icon icon, Clay_String text, bool enabled,
		ButtonStyle style)
	{
		bool hovered, pressed;
		bool clicked = track(id, enabled, hovered, pressed);
		Clay_Color background = { 0, 0, 0, 0 };
		Clay_Color color = enabled ? InkColor : FaintColor;
		if(style == ButtonPrimary)
		{
			background = pressed ? AccentPressedColor : hovered ? AccentHoverColor : AccentColor;
			color = HighlightTextColor;
		}
		else if(style == ButtonAccent)
		{
			background = pressed ? PressedColor : hovered ? AccentLightHoverColor :
				AccentLightColor;
			color = AccentHoverColor;
		}
		else if(pressed)
			background = PressedColor;
		else if(hovered)
			background = HoverColor;
		CLAY({ .id = id,
			.layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(36) },
				.padding = { 10, 10, 0, 0 }, .childGap = 10,
				.childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER } },
			.backgroundColor = background,
			.cornerRadius = CLAY_CORNER_RADIUS(8) })
		{
			CLAY({ .layout = { .sizing = { CLAY_SIZING_FIXED(18), CLAY_SIZING_FIXED(18) } },
				.custom = { .customData = icons::custom(icon, toColor(color)) } }) {}
			CLAY_TEXT(text, textConfig(PanelFontSize, color));
		}
		return clicked;
	}

	bool menuItem(Clay_ElementId id, CustomDraw* icon, Clay_String text, bool checked,
		bool enabled, Clay_Color color)
	{
		bool hovered, pressed;
		bool clicked = track(id, enabled, hovered, pressed);
		Clay_Color background = { 0, 0, 0, 0 };
		if(pressed)
			background = PressedColor;
		else if(hovered)
			background = HoverColor;
		else if(checked)
			background = AccentLightColor;
		CLAY({ .id = id,
			.layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(32) },
				.padding = { 8, 8, 0, 0 }, .childGap = 10,
				.childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER } },
			.backgroundColor = background,
			.cornerRadius = CLAY_CORNER_RADIUS(6) })
		{
			CLAY({ .layout = { .sizing = { CLAY_SIZING_FIXED(30), CLAY_SIZING_FIXED(20) } },
				.custom = { .customData = icon } }) {}
			CLAY_TEXT(text, textConfig(PanelFontSize, enabled ? color : FaintColor));
			CLAY({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } } }) {}
			if(checked)
				CLAY({ .layout = { .sizing = { CLAY_SIZING_FIXED(14), CLAY_SIZING_FIXED(14) } },
					.custom = { .customData = icons::custom(icons::Check, toColor(AccentColor)) } }) {}
		}
		return clicked;
	}

	void separator(float margin)
	{
		CLAY({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
			.padding = { 4, 4, (uint16_t)margin, (uint16_t)margin } } })
		{
			CLAY({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
				.backgroundColor = PanelBorderColor }) {}
		}
	}

	void groupTitle(Clay_String text)
	{
		CLAY({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
			.padding = { 8, 8, 4, 6 } } })
		{
			CLAY_TEXT(text, textConfig(CaptionFontSize, MutedColor));
		}
	}

	/**
	 * Caractere Latin-1 formado pela letra base e o acento combinante, ou 0
	 */
	static int compose(int base, int mark)
	{
		static const struct
		{
			int mark;
			const char* bases;
			const char* results;
		} table[] = {
			{ 0x300, "AEIOUaeiou", "\xC0\xC8\xCC\xD2\xD9\xE0\xE8\xEC\xF2\xF9" },
			{ 0x301, "AEIOUYaeiouy", "\xC1\xC9\xCD\xD3\xDA\xDD\xE1\xE9\xED\xF3\xFA\xFD" },
			{ 0x302, "AEIOUaeiou", "\xC2\xCA\xCE\xD4\xDB\xE2\xEA\xEE\xF4\xFB" },
			{ 0x303, "ANOano", "\xC3\xD1\xD5\xE3\xF1\xF5" },
			{ 0x308, "AEIOUaeiouy", "\xC4\xCB\xCF\xD6\xDC\xE4\xEB\xEF\xF6\xFC\xFF" },
			{ 0x30A, "Aa", "\xC5\xE5" },
			{ 0x327, "Cc", "\xC7\xE7" },
		};
		if(base <= 0 || base >= 128)
			return 0;
		for(const auto& entry : table)
		{
			if(entry.mark != mark)
				continue;
			const char* found = strchr(entry.bases, base);
			if(found == NULL)
				return 0;
			return (unsigned char)entry.results[found - entry.bases];
		}
		return 0;
	}

	std::string composeLatin1(const std::string& text)
	{
		std::string result;
		size_t lastStart = 0;
		int last = 0;
		for(size_t i = 0; i < text.size();)
		{
			int size;
			int codepoint = GetCodepointNext(text.c_str() + i, &size);
			int composed = compose(last, codepoint);
			if(composed != 0)
			{
				int length;
				const char* utf8 = CodepointToUTF8(composed, &length);
				result.resize(lastStart);
				result.append(utf8, length);
				last = composed;
			}
			else
			{
				lastStart = result.size();
				result.append(text, i, size);
				last = codepoint;
			}
			i += size;
		}
		return result;
	}

}

static bool keyPressed(int key)
{
	return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

static bool commandDown()
{
	// Cmd no macOS, Ctrl nos demais
	return IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER) ||
		IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
}

TextField::TextField()
{
	cursor = 0;
	allSelected = false;
	fontSize = ui::FontSize;
	id = CLAY_ID("TextField");
}

void TextField::setText(const std::string& text)
{
	this->text = text;
	cursor = text.size();
	allSelected = false;
}

const std::string& TextField::getText()
{
	return text;
}

void TextField::selectAll()
{
	allSelected = !text.empty();
	cursor = text.size();
}

size_t TextField::previous(size_t pos)
{
	// pula os bytes de continuação do UTF-8
	do
		pos--;
	while(pos > 0 && (text[pos] & 0xC0) == 0x80);
	return pos;
}

size_t TextField::next(size_t pos)
{
	do
		pos++;
	while(pos < text.size() && (text[pos] & 0xC0) == 0x80);
	return pos;
}

void TextField::clear()
{
	text.clear();
	cursor = 0;
	allSelected = false;
}

void TextField::insert(const std::string& str)
{
	if(allSelected)
		clear();
	text.insert(cursor, str);
	cursor += str.size();
}

void TextField::moveCursorTo(float x)
{
	Clay_ElementData data = Clay_GetElementData(id);
	if(!data.found)
		return;
	float offset = x - data.boundingBox.x - 8;
	size_t best = 0;
	float bestDistance = fabsf(offset);
	for(size_t pos = 0; pos < text.size();)
	{
		pos = next(pos);
		std::string prefix(text, 0, pos);
		float width = MeasureTextEx(getFont(fontSize), prefix.c_str(), fontSize, 0).x;
		if(fabsf(offset - width) < bestDistance)
		{
			best = pos;
			bestDistance = fabsf(offset - width);
		}
	}
	allSelected = false;
	cursor = best;
}

bool TextField::update()
{
	if(commandDown())
	{
		if(IsKeyPressed(KEY_A))
			selectAll();
		else if(IsKeyPressed(KEY_C) && allSelected)
			SetClipboardText(text.c_str());
		else if(IsKeyPressed(KEY_X) && allSelected)
		{
			SetClipboardText(text.c_str());
			clear();
		}
		else if(IsKeyPressed(KEY_V))
		{
			const char* clipboard = GetClipboardText();
			if(clipboard != NULL)
			{
				std::string line(clipboard);
				line = line.substr(0, line.find_first_of("\r\n"));
				if(!line.empty())
					insert(line);
			}
		}
	}
	int codepoint;
	while((codepoint = GetCharPressed()) > 0)
	{
		if(codepoint < 32 || codepoint == 127)
			continue;
		int size;
		const char* utf8 = CodepointToUTF8(codepoint, &size);
		insert(std::string(utf8, size));
	}
	if(keyPressed(KEY_BACKSPACE))
	{
		if(allSelected)
			clear();
		else if(cursor > 0)
		{
			size_t pos = previous(cursor);
			text.erase(pos, cursor - pos);
			cursor = pos;
		}
	}
	if(keyPressed(KEY_DELETE))
	{
		if(allSelected)
			clear();
		else if(cursor < text.size())
			text.erase(cursor, next(cursor) - cursor);
	}
	if(keyPressed(KEY_LEFT))
	{
		if(allSelected || commandDown())
			cursor = 0;
		else if(cursor > 0)
			cursor = previous(cursor);
		allSelected = false;
	}
	if(keyPressed(KEY_RIGHT))
	{
		if(allSelected || commandDown())
			cursor = text.size();
		else if(cursor < text.size())
			cursor = next(cursor);
		allSelected = false;
	}
	if(IsKeyPressed(KEY_HOME))
	{
		cursor = 0;
		allSelected = false;
	}
	if(IsKeyPressed(KEY_END))
	{
		cursor = text.size();
		allSelected = false;
	}
	return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
}

void TextField::layout(Clay_ElementId id, uint16_t fontSize, float height, float radius)
{
	this->id = id;
	this->fontSize = fontSize;
	if(Clay_PointerOver(id) && input::mousePressed())
		moveCursorTo(GetMousePosition().x);
	CLAY({ .id = id,
		.layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(height) },
			.padding = { 8, 8, 0, 0 },
			.childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER } },
		.backgroundColor = ui::FieldColor,
		.cornerRadius = CLAY_CORNER_RADIUS(radius),
		.clip = { .horizontal = true },
		.border = { .color = ui::FocusColor, .width = CLAY_BORDER_OUTSIDE(2) } })
	{
		if(allSelected)
		{
			CLAY({ .backgroundColor = ui::SelectionColor })
			{
				CLAY_TEXT(toClayString(text), ui::textConfig(fontSize));
			}
		}
		else
		{
			before.assign(text, 0, cursor);
			after.assign(text, cursor, std::string::npos);
			if(!before.empty())
				CLAY_TEXT(toClayString(before), ui::textConfig(fontSize));
			CLAY({ .layout = { .sizing = { CLAY_SIZING_FIXED(1),
				CLAY_SIZING_FIXED((float)fontSize + 3) } },
				.backgroundColor = ui::TextColor }) {}
			if(!after.empty())
				CLAY_TEXT(toClayString(after), ui::textConfig(fontSize));
		}
	}
}

MessageDialog::MessageDialog()
{
	visible = false;
}

void MessageDialog::show(const std::string& title, const std::string& message)
{
	this->title = ui::composeLatin1(title);
	this->message = ui::composeLatin1(message);
	visible = true;
}

bool MessageDialog::isVisible()
{
	return visible;
}

void MessageDialog::update()
{
	if(IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
		visible = false;
}

void MessageDialog::layout()
{
	ui::dialog(toClayString(title), 360, [&]()
	{
		CLAY_TEXT(toClayString(message), ui::textConfig(ui::FontSize, ui::TextColor,
			CLAY_TEXT_WRAP_WORDS));
		ui::dialogButtons([&]()
		{
			if(ui::button(CLAY_ID("MessageOk"), CLAY_STRING("OK"), true, 80))
				visible = false;
		});
	});
}

ConfirmDialog::ConfirmDialog()
{
	visible = false;
	confirmed = false;
}

void ConfirmDialog::show(const std::string& title, const std::string& message,
	const std::string& confirmText)
{
	this->title = ui::composeLatin1(title);
	this->message = ui::composeLatin1(message);
	this->confirmText = ui::composeLatin1(confirmText);
	visible = true;
	confirmed = false;
}

bool ConfirmDialog::isVisible()
{
	return visible;
}

bool ConfirmDialog::takeConfirmed()
{
	bool result = confirmed;
	confirmed = false;
	return result;
}

void ConfirmDialog::close(bool confirmed)
{
	this->confirmed = confirmed;
	visible = false;
}

void ConfirmDialog::update()
{
	if(IsKeyPressed(KEY_ESCAPE))
		close(false);
	else if(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
		close(true);
}

void ConfirmDialog::layout()
{
	ui::dialog(toClayString(title), 360, [&]()
	{
		CLAY_TEXT(toClayString(message), ui::textConfig(ui::FontSize, ui::TextColor,
			CLAY_TEXT_WRAP_WORDS));
		ui::dialogButtons([&]()
		{
			if(ui::button(CLAY_ID("ConfirmOk"), toClayString(confirmText), true, 80))
				close(true);
			if(ui::button(CLAY_ID("ConfirmCancelar"), CLAY_STRING("Cancelar"), true, 80))
				close(false);
		});
	});
}
