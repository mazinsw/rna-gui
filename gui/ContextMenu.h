#ifndef _CONTEXTMENU_H_
#define _CONTEXTMENU_H_
#include "Widgets.h"
#include "NeuralNetwork.h"

/**
 * Menu do clique direito: a função de ativação e a exclusão da camada ou do
 * neurônio, e nas entradas a imagem que elas formam
 */
class ContextMenu
{
private:
	NeuralNetwork * network;
	// camada, -1 nas entradas
	int layer;
	Vector2 position;
	bool visible;
private:
	/**
	 * Item de exclusão, desativado durante a execução
	 */
	bool deleteItem(Clay_ElementId id, Clay_String text)
	{
		bool enabled = !network->isRunning();
		Color color = ui::toColor(enabled ? ui::DangerColor : ui::FaintColor);
		return ui::menuItem(id, icons::custom(icons::Trash, color), text, false, enabled,
			ui::DangerColor);
	}
	void layerItems()
	{
		static const Clay_String names[] = {
			CLAY_STRING("Linear"),
			CLAY_STRING("Degrau"),
			CLAY_STRING("Bipolar"),
			CLAY_STRING("Logística"),
			CLAY_STRING("Hiperbólica"),
			CLAY_STRING("ReLU"),
			CLAY_STRING("Softmax"),
		};
		int function = network->getNetwork().layers[layer].function;
		ui::groupTitle(CLAY_STRING("FUNÇÃO DA CAMADA"));
		for(int i = 0; i < Network::FunctionCount; i++)
		{
			if(ui::menuItem(CLAY_IDI("MenuFuncao", i), icons::customFunction(i), names[i],
				function == i))
			{
				network->setFunction(layer, i);
				close();
			}
		}
		ui::separator(4);
		if(network->getSelected().kind == NeuralNetwork::Part::Neuron &&
			deleteItem(CLAY_ID("MenuExcluirNeuronio"), CLAY_STRING("Excluir neurônio")))
		{
			network->removeSelected();
			close();
		}
		if(deleteItem(CLAY_ID("MenuExcluirCamada"), CLAY_STRING("Excluir camada")))
		{
			network->removeLayer(layer);
			close();
		}
	}
	void inputItems()
	{
		static const int sides[] = { 0, 8, 16, 28 };
		static const Clay_String names[] = {
			CLAY_STRING("Entradas soltas"),
			CLAY_STRING("Imagem 8 × 8"),
			CLAY_STRING("Imagem 16 × 16"),
			CLAY_STRING("Imagem 28 × 28"),
		};
		Color color = ui::toColor(network->isRunning() ? ui::FaintColor : ui::NodeColor);
		int side = network->getImageSide();
		ui::groupTitle(CLAY_STRING("ENTRADAS"));
		for(int i = 0; i < 4; i++)
		{
			icons::Icon icon = sides[i] == 0 ? icons::InputData : icons::Image;
			if(ui::menuItem(CLAY_IDI("MenuImagem", i), icons::custom(icon, color), names[i],
				side == sides[i], !network->isRunning()))
			{
				network->setImageSide(sides[i]);
				close();
			}
		}
		if(network->getSelected().kind != NeuralNetwork::Part::Input)
			return;
		ui::separator(4);
		if(deleteItem(CLAY_ID("MenuExcluirEntrada"), CLAY_STRING("Excluir entrada")))
		{
			network->removeSelected();
			close();
		}
	}
public:
	ContextMenu()
	{
		network = NULL;
		layer = -1;
		position = Vector2{ 0, 0 };
		visible = false;
	}
	/**
	 * Abre o menu da camada ou das entradas (-1), position é o canto do menu
	 * na janela em unidades da interface
	 */
	void show(NeuralNetwork * network, int layer, Vector2 position)
	{
		this->network = network;
		this->layer = layer;
		this->position = position;
		visible = true;
	}
	bool isVisible()
	{
		return visible;
	}
	void close()
	{
		visible = false;
	}
	void layout()
	{
		// a camada pode ter sido excluída por outro caminho
		if(layer >= (int)network->getNetwork().layers.size())
		{
			close();
			return;
		}
		// mantém o menu dentro da janela com o tamanho do quadro anterior
		Clay_ElementData data = Clay_GetElementData(CLAY_ID("Menu"));
		Vector2 offset = position;
		if(data.found)
		{
			Clay_Dimensions window = ClayRaylib_GetLayoutDimensions();
			float right = window.width - data.boundingBox.width - 8;
			float bottom = window.height - data.boundingBox.height - 8;
			offset.x = offset.x > right ? right : offset.x;
			offset.y = offset.y > bottom ? bottom : offset.y;
		}
		CLAY({ .id = CLAY_ID("Menu"),
			.layout = { .sizing = { CLAY_SIZING_FIXED(216), CLAY_SIZING_FIT(0) },
				.padding = CLAY_PADDING_ALL(6), .childGap = 1,
				.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.backgroundColor = ui::PanelColor,
			.cornerRadius = CLAY_CORNER_RADIUS(10),
			.floating = { .offset = { offset.x, offset.y }, .zIndex = 20,
				.attachTo = CLAY_ATTACH_TO_ROOT },
			.border = { .color = ui::PanelBorderColor, .width = CLAY_BORDER_OUTSIDE(1) } })
		{
			if(layer >= 0)
				layerItems();
			else
				inputItems();
		}
	}
};

#endif /* _CONTEXTMENU_H_ */
