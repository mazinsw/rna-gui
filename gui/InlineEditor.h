#ifndef _INLINEEDITOR_H_
#define _INLINEEDITOR_H_
#include "Widgets.h"
#include "NeuralNetwork.h"
#include "Number.h"

/**
 * Edita no lugar o peso ou o bias selecionado: Enter ou clique fora aplica,
 * Esc cancela
 */
class InlineEditor
{
private:
	// rótulo editado, na rede
	Rectangle label;
	TextField field;
	bool visible;
public:
	InlineEditor()
	{
		label = Rectangle{ 0, 0, 0, 0 };
		visible = false;
	}
	void show(Rectangle label, float value)
	{
		this->label = label;
		field.setText(number::format(value, 6));
		field.selectAll();
		visible = true;
	}
	bool isVisible()
	{
		return visible;
	}
	/**
	 * Fecha e lê o valor digitado, retorna false se o texto não é número
	 */
	bool commit(float& value)
	{
		double number;
		visible = false;
		if(!number::parse(field.getText(), number))
			return false;
		value = (float)number;
		return true;
	}
	void cancel()
	{
		visible = false;
	}
	/**
	 * Processa o teclado, retorna true com o valor quando Enter o aplica
	 */
	bool update(float& value)
	{
		if(IsKeyPressed(KEY_ESCAPE))
		{
			cancel();
			return false;
		}
		if(field.update())
			return commit(value);
		return false;
	}
	/**
	 * Campo sobre o rótulo, origin é o canto da área de edição na janela e a
	 * rede converte a posição do rótulo com o zoom da vista
	 */
	void layout(Vector2 origin, NeuralNetwork * network)
	{
		Rectangle box = network->toScreen(label);
		// o rótulo fica na janela, o campo em unidades da interface
		Vector2 center = ClayRaylib_ToLayout(Vector2{ origin.x + box.x + box.width / 2,
			origin.y + box.y + box.height / 2 });
		float width = ClayRaylib_ToLayout(Vector2{ box.width, 0 }).x + 24;
		if(width < 84)
			width = 84;
		// centralizado no rótulo, a dica abaixo do campo pode ser mais larga que ele
		CLAY({ .id = CLAY_ID("Editor"),
			.layout = { .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
				.childGap = 6, .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_TOP },
				.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.floating = { .offset = { center.x, center.y - 13 }, .zIndex = 15,
				.attachPoints = { .element = CLAY_ATTACH_POINT_CENTER_TOP,
					.parent = CLAY_ATTACH_POINT_LEFT_TOP },
				.attachTo = CLAY_ATTACH_TO_ROOT } })
		{
			CLAY({ .layout = { .sizing = { CLAY_SIZING_FIXED(width), CLAY_SIZING_FIT(0) } } })
			{
				field.layout(CLAY_ID("EditorField"), ui::LabelFontSize, 26, 13);
			}
			CLAY({ .layout = { .padding = { 8, 8, 3, 3 } },
				.backgroundColor = ui::InkColor,
				.cornerRadius = CLAY_CORNER_RADIUS(6) })
			{
				CLAY_TEXT(CLAY_STRING("Enter aplica · Esc cancela"),
					ui::textConfig(ui::CaptionFontSize, ui::HighlightTextColor));
			}
		}
	}
};

#endif /* _INLINEEDITOR_H_ */
