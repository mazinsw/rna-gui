#ifndef _WIDGETS_H_
#define _WIDGETS_H_
#include <string>
#include <vector>
#include "ClayRaylib.h"

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
	const Clay_Color PrimaryHoverColor = { 0, 108, 230, 255 };
	const Clay_Color PrimaryPressedColor = { 0, 94, 200, 255 };
	const Clay_Color AccentTextColor = { 0, 96, 200, 255 };
	const Clay_Color StatusTextColor = { 80, 80, 80, 255 };
	const Clay_Color SuccessColor = { 21, 128, 61, 255 };
	const Clay_Color WarningColor = { 194, 65, 12, 255 };

	/**
	 * Aparência do botão
	 */
	enum ButtonStyle
	{
		// fundo branco
		ButtonDefault,
		// fundo azul, para um botão ligado
		ButtonPrimary,
		// borda e texto azuis, para a ação principal do momento
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
	bool button(Clay_ElementId id, Clay_String text, bool enabled = true, float minWidth = 0,
		ButtonStyle style = ButtonDefault);

	void label(Clay_String text, uint16_t fontSize = FontSize);

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
	void layout(Clay_ElementId id, uint16_t fontSize = ui::FontSize);
};

/**
 * Lista de opções que abre abaixo do controle
 */
class ComboBox
{
private:
	std::vector<std::string> items;
	int selected;
	bool open;
public:
	ComboBox();
	void add(const std::string& item);
	int getSelected();
	void setSelected(int index);

	/**
	 * Fecha a lista se estiver aberta, retorna true se fechou
	 */
	bool close();
	bool isOpen();

	/**
	 * Processa o teclado, retorna true quando Enter foi pressionado com a
	 * lista fechada
	 */
	bool update();
	void layout(Clay_ElementId id, Clay_ElementId listId, uint16_t fontSize = ui::FontSize);
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
