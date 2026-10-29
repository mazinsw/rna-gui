#ifndef _INLINEEDITOR_H_
#define _INLINEEDITOR_H_
#include "Widgets.h"
#include "Edge.h"
#include "NeuronNode.h"
#include "Resources.h"

/**
 * Edita no lugar o peso de uma conexão ou o bias de um neurônio: Enter ou
 * clique fora aplica, Esc cancela
 */
class InlineEditor
{
private:
	Edge * edge;
	NeuronNode * neuron;
	TextField field;
	bool visible;
private:
	void open(double value)
	{
		field.setText(Node::formatNumber(value, 6));
		field.selectAll();
		visible = true;
	}
public:
	InlineEditor()
	{
		edge = NULL;
		neuron = NULL;
		visible = false;
	}
	void show(Edge * edge)
	{
		this->edge = edge;
		neuron = NULL;
		open(edge->getWeight());
	}
	void show(NeuronNode * neuron)
	{
		this->neuron = neuron;
		edge = NULL;
		open(neuron->getBias());
	}
	bool isVisible()
	{
		return visible;
	}
	/**
	 * Aplica o valor digitado, retorna true se mudou. Um texto que não é
	 * número mantém o valor anterior.
	 */
	bool commit()
	{
		double value;
		visible = false;
		if(!Node::parseNumber(field.getText(), value))
			return false;
		if(edge != NULL)
		{
			if(value == edge->getWeight())
				return false;
			edge->setWeight(value);
		}
		else
		{
			if(value == neuron->getBias())
				return false;
			neuron->setBias(value);
		}
		return true;
	}
	void cancel()
	{
		visible = false;
	}
	/**
	 * Processa o teclado, retorna true se o valor foi aplicado e mudou
	 */
	bool update()
	{
		if(IsKeyPressed(KEY_ESCAPE))
		{
			cancel();
			return false;
		}
		if(field.update())
			return commit();
		return false;
	}
	/**
	 * Campo sobre o rótulo, origin é o canto da área de edição na janela
	 */
	void layout(Vector2 origin)
	{
		Rectangle label = edge != NULL ? edge->getLabelBox() : neuron->getBiasBox();
		float width = label.width + 24;
		if(width < 84)
			width = 84;
		// a dica abaixo do campo pode ser mais larga que ele
		float hintWidth = MeasureTextEx(getFont(), "Enter aplica · Esc cancela", 11, 0).x + 16;
		float outer = hintWidth > width ? hintWidth : width;
		float x = origin.x + label.x + label.width / 2 - outer / 2;
		float y = origin.y + label.y + label.height / 2 - 12;
		CLAY({ .id = CLAY_ID("Editor"),
			.layout = { .sizing = { CLAY_SIZING_FIXED(outer), CLAY_SIZING_FIT(0) },
				.childGap = 6, .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_TOP },
				.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.floating = { .offset = { x, y }, .zIndex = 15, .attachTo = CLAY_ATTACH_TO_ROOT } })
		{
			CLAY({ .layout = { .sizing = { CLAY_SIZING_FIXED(width), CLAY_SIZING_FIT(0) } } })
			{
				field.layout(CLAY_ID("EditorField"), 12, 24, 12);
			}
			CLAY({ .layout = { .padding = { 8, 8, 3, 3 } },
				.backgroundColor = ui::InkColor,
				.cornerRadius = CLAY_CORNER_RADIUS(6) })
			{
				CLAY_TEXT(CLAY_STRING("Enter aplica · Esc cancela"),
					ui::textConfig(11, ui::HighlightTextColor));
			}
		}
	}
};

#endif /* _INLINEEDITOR_H_ */
