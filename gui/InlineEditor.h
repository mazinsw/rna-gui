#ifndef _INLINEEDITOR_H_
#define _INLINEEDITOR_H_
#include "Widgets.h"
#include "Edge.h"
#include "NeuralNetwork.h"
#include "NeuronNode.h"

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
	 * Campo sobre o rótulo, origin é o canto da área de edição na janela e a
	 * rede converte a posição do rótulo com o zoom da vista
	 */
	void layout(Vector2 origin, NeuralNetwork * network)
	{
		Rectangle label = network->toScreen(edge != NULL ? edge->getLabelBox() :
			neuron->getBiasBox());
		// o rótulo fica na janela, o campo em unidades da interface
		Vector2 center = ClayRaylib_ToLayout(Vector2{ origin.x + label.x + label.width / 2,
			origin.y + label.y + label.height / 2 });
		float width = ClayRaylib_ToLayout(Vector2{ label.width, 0 }).x + 24;
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
