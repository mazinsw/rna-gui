#ifndef _WIDGETS_H_
#define _WIDGETS_H_
#include <string>
#include <vector>
#include "ClayRaylib.h"
#include "Icons.h"

/**
 * Controles desenhados com o Clay, em modo imediato: cada quadro declara a
 * interface novamente e as funções retornam as interações do usuário
 */
namespace ui
{
	const uint16_t FontSize = 13;
	const float ButtonHeight = 26;
	const Clay_Color WindowColor = { 236, 236, 236, 255 };
	const Clay_Color TextColor = { 30, 30, 30, 255 };
	const Clay_Color DisabledTextColor = { 160, 160, 160, 255 };
	const Clay_Color ButtonColor = { 255, 255, 255, 255 };
	const Clay_Color ButtonHoverColor = { 246, 246, 246, 255 };
	const Clay_Color ButtonPressedColor = { 222, 222, 222, 255 };
	const Clay_Color BorderColor = { 192, 192, 192, 255 };
	const Clay_Color FieldColor = { 255, 255, 255, 255 };
	const Clay_Color FocusColor = { 0, 122, 255, 255 };
	const Clay_Color SelectionColor = { 179, 215, 255, 255 };
	const Clay_Color HighlightTextColor = { 255, 255, 255, 255 };
	const Clay_Color DialogColor = { 246, 246, 246, 255 };
	const Clay_Color OverlayColor = { 0, 0, 0, 70 };
	// área de edição e painéis flutuantes
	const Clay_Color CanvasColor = { 248, 250, 252, 255 };
	const Clay_Color GridColor = { 213, 220, 230, 255 };
	const Clay_Color PanelColor = { 255, 255, 255, 255 };
	const Clay_Color PanelBorderColor = { 226, 232, 240, 255 };
	const Clay_Color HoverColor = { 241, 245, 249, 255 };
	const Clay_Color PressedColor = { 226, 232, 240, 255 };
	const Clay_Color InkColor = { 15, 23, 42, 255 };
	const Clay_Color NodeColor = { 51, 65, 85, 255 };
	const Clay_Color MutedColor = { 100, 116, 139, 255 };
	const Clay_Color StatusTextColor = { 71, 85, 105, 255 };
	const Clay_Color FaintColor = { 148, 163, 184, 255 };
	const Clay_Color PillBorderColor = { 203, 213, 225, 255 };
	const Clay_Color AccentColor = { 37, 99, 235, 255 };
	const Clay_Color AccentHoverColor = { 29, 78, 216, 255 };
	const Clay_Color AccentPressedColor = { 30, 64, 175, 255 };
	const Clay_Color AccentLightColor = { 239, 246, 255, 255 };
	const Clay_Color AccentLightHoverColor = { 219, 234, 254, 255 };
	const Clay_Color AccentDarkColor = { 30, 58, 138, 255 };
	const Clay_Color SelectionRingColor = { 37, 99, 235, 90 };
	const Clay_Color SuccessColor = { 21, 128, 61, 255 };
	const Clay_Color SuccessLightColor = { 220, 252, 231, 255 };
	const Clay_Color WarningColor = { 194, 65, 12, 255 };
	const Clay_Color WarningLightColor = { 255, 237, 213, 255 };
	const Clay_Color ChangedColor = { 254, 243, 199, 255 };
	const Clay_Color ChangedBorderColor = { 245, 158, 11, 255 };
	const Clay_Color DangerColor = { 185, 28, 28, 255 };

	/**
	 * Aparência do botão do painel
	 */
	enum ButtonStyle
	{
		// fundo transparente
		ButtonDefault,
		// fundo azul, para um botão ligado
		ButtonPrimary,
		// fundo azul claro, para a ação principal do momento
		ButtonAccent
	};

	inline Color toColor(Clay_Color color)
	{
		return Color{ (unsigned char)color.r, (unsigned char)color.g, (unsigned char)color.b,
			(unsigned char)color.a };
	}

	/**
	 * Finaliza o quadro, chamar após Clay_EndLayout
	 */
	void endFrame();

	/**
	 * Configuração de texto com a fonte da interface
	 */
	Clay_TextElementConfig* textConfig(uint16_t fontSize = FontSize, Clay_Color color = TextColor,
		Clay_TextElementConfigWrapMode wrapMode = CLAY_TEXT_WRAP_NONE);

	/**
	 * Botão, retorna true quando clicado
	 */
	bool button(Clay_ElementId id, Clay_String text, bool enabled = true, float minWidth = 0);

	/**
	 * Botão do painel com ícone e texto, ocupa a largura do painel
	 */
	bool toolButton(Clay_ElementId id, icons::Icon icon, Clay_String text, bool enabled = true,
		ButtonStyle style = ButtonDefault);

	/**
	 * Item de menu com ícone, marcado quando checked
	 */
	bool menuItem(Clay_ElementId id, CustomDraw* icon, Clay_String text, bool checked = false,
		bool enabled = true, Clay_Color color = InkColor);

	/**
	 * Linha horizontal entre grupos de um painel ou menu
	 */
	void separator(float margin);

	/**
	 * Título de um grupo do painel ou do menu
	 */
	void groupTitle(Clay_String text);

	/**
	 * Junta letra + acento combinante (Unicode decomposto, como o macOS
	 * entrega nomes de arquivo) no caractere Latin-1 equivalente, que é o que
	 * a fonte da interface contém
	 */
	std::string composeLatin1(const std::string& text);

	/**
	 * Janela modal centralizada sobre a interface, o conteúdo é declarado
	 * pela função body
	 */
	template<class Body>
	void dialog(Clay_String title, float width, Body body)
	{
		Clay_Sizing screen = { CLAY_SIZING_FIXED((float)GetScreenWidth()),
			CLAY_SIZING_FIXED((float)GetScreenHeight()) };
		CLAY({ .id = CLAY_ID("DialogOverlay"),
			.layout = { .sizing = screen,
				.childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER } },
			.backgroundColor = OverlayColor,
			.floating = { .zIndex = 10, .attachTo = CLAY_ATTACH_TO_ROOT } })
		{
			CLAY({ .id = CLAY_ID("Dialog"),
				.layout = { .sizing = { CLAY_SIZING_FIXED(width), CLAY_SIZING_FIT(0) },
					.layoutDirection = CLAY_TOP_TO_BOTTOM },
				.backgroundColor = DialogColor,
				.cornerRadius = CLAY_CORNER_RADIUS(8),
				.border = { .color = BorderColor, .width = CLAY_BORDER_OUTSIDE(1) } })
			{
				CLAY({ .id = CLAY_ID("DialogTitle"),
					.layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
						.padding = { 12, 12, 10, 10 },
						.childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER } },
					.border = { .color = BorderColor, .width = { .bottom = 1 } } })
				{
					CLAY_TEXT(title, textConfig());
				}
				CLAY({ .id = CLAY_ID("DialogBody"),
					.layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
						.padding = CLAY_PADDING_ALL(10), .childGap = 10,
						.layoutDirection = CLAY_TOP_TO_BOTTOM } })
				{
					body();
				}
			}
		}
	}

	/**
	 * Linha de botões alinhada à direita no rodapé de um diálogo
	 */
	template<class Body>
	void dialogButtons(Body body)
	{
		CLAY({ .id = CLAY_ID("DialogButtons"),
			.layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
				.padding = { 0, 0, 10, 0 }, .childGap = 6,
				.childAlignment = { CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER } } })
		{
			body();
		}
	}
}

/**
 * Campo de texto de uma linha
 */
class TextField
{
private:
	std::string text;
	size_t cursor;
	bool allSelected;
	uint16_t fontSize;
	// trechos antes e depois do cursor, vivos até o fim do quadro
	std::string before;
	std::string after;
	Clay_ElementId id;
private:
	size_t previous(size_t pos);
	size_t next(size_t pos);
	void insert(const std::string& str);
	void clear();
	void moveCursorTo(float x);
public:
	TextField();
	void setText(const std::string& text);
	const std::string& getText();
	void selectAll();

	/**
	 * Processa o teclado, retorna true quando Enter foi pressionado
	 */
	bool update();
	void layout(Clay_ElementId id, uint16_t fontSize = ui::FontSize, float height = 30,
		float radius = 4);
};

/**
 * Mensagem com botão OK
 */
class MessageDialog
{
private:
	std::string title;
	std::string message;
	bool visible;
public:
	MessageDialog();
	void show(const std::string& title, const std::string& message);
	bool isVisible();
	void update();
	void layout();
};

/**
 * Pergunta com botões de confirmar e cancelar
 */
class ConfirmDialog
{
private:
	std::string title;
	std::string message;
	std::string confirmText;
	bool visible;
	bool confirmed;
private:
	void close(bool confirmed);
public:
	ConfirmDialog();
	void show(const std::string& title, const std::string& message,
		const std::string& confirmText);
	bool isVisible();

	/**
	 * Retorna true uma única vez depois que o usuário confirmou
	 */
	bool takeConfirmed();
	void update();
	void layout();
};

#endif /* _WIDGETS_H_ */
