#ifndef _CONTEXTMENU_H_
#define _CONTEXTMENU_H_
#include "Widgets.h"
#include "NeuralNetwork.h"

/**
 * Menu do clique direito: função de ativação do neurônio, tipo da entrada e
 * exclusão do nó ou da conexão
 */
class ContextMenu
{
private:
	NeuralNetwork * network;
	Node * node;
	Edge * edge;
	Vector2 position;
	bool visible;
private:
	/**
	 * Item de exclusão, desativado durante a execução
	 */
	void deleteItem(Clay_String text)
	{
		bool enabled = !network->isRunning();
		Color color = ui::toColor(enabled ? ui::DangerColor : ui::FaintColor);
		ui::separator(4);
		if(!ui::menuItem(CLAY_ID("MenuExcluir"), icons::custom(icons::Trash, color), text, false,
			enabled, ui::DangerColor))
			return;
		if(node != NULL)
			network->removeNode(node);
		else
			network->removeEdge(edge);
		close();
	}
	void neuronItems(NeuronNode * neuron)
	{
		static const Clay_String names[] = {
			CLAY_STRING("Linear"),
			CLAY_STRING("Degrau"),
			CLAY_STRING("Bipolar"),
			CLAY_STRING("Logística"),
			CLAY_STRING("Hiperbólica"),
		};
		ui::groupTitle(CLAY_STRING("FUNÇÃO DE ATIVAÇÃO"));
		for(int i = 0; i < NeuronNode::FunctionCount; i++)
		{
			if(ui::menuItem(CLAY_IDI("MenuFuncao", i), icons::customFunction(i), names[i],
				neuron->getFunctionType() == i))
			{
				network->setFunction(neuron, i);
				close();
			}
		}
		deleteItem(CLAY_STRING("Excluir neurônio"));
	}
	void inputItems(InputNode * input)
	{
		Color color = ui::toColor(ui::NodeColor);
		ui::groupTitle(CLAY_STRING("TIPO DA ENTRADA"));
		if(ui::menuItem(CLAY_ID("MenuDado"), icons::custom(icons::InputData, color),
			CLAY_STRING("Dado (0 ou 1)"), !input->isBias()))
		{
			network->setInputBias(input, false);
			close();
		}
		if(ui::menuItem(CLAY_ID("MenuBias"), icons::custom(icons::InputBias, color),
			CLAY_STRING("Bias (sempre 1)"), input->isBias()))
		{
			network->setInputBias(input, true);
			close();
		}
		deleteItem(CLAY_STRING("Excluir entrada"));
	}
public:
	ContextMenu()
	{
		network = NULL;
		node = NULL;
		edge = NULL;
		position = Vector2{ 0, 0 };
		visible = false;
	}
	/**
	 * Abre o menu do nó ou da conexão, position é o canto do menu na janela
	 */
	void show(NeuralNetwork * network, Node * node, Edge * edge, Vector2 position)
	{
		this->network = network;
		this->node = node;
		this->edge = edge;
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
		// mantém o menu dentro da janela com o tamanho do quadro anterior
		Clay_ElementData data = Clay_GetElementData(CLAY_ID("Menu"));
		Vector2 offset = position;
		if(data.found)
		{
			float right = GetScreenWidth() - data.boundingBox.width - 8;
			float bottom = GetScreenHeight() - data.boundingBox.height - 8;
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
			if(node != NULL && node->is(Node::TypeNeuron))
				neuronItems((NeuronNode*)node);
			else if(node != NULL && node->is(Node::TypeInput))
				inputItems((InputNode*)node);
			else if(node != NULL)
			{
				ui::groupTitle(CLAY_STRING("SAÍDA"));
				deleteItem(CLAY_STRING("Excluir saída"));
			}
			else
			{
				ui::groupTitle(CLAY_STRING("CONEXÃO"));
				deleteItem(CLAY_STRING("Excluir conexão"));
			}
		}
	}
};

#endif /* _CONTEXTMENU_H_ */
