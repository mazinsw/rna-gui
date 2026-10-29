#ifndef _NODEPROPERTIES_H_
#define _NODEPROPERTIES_H_
#include "Widgets.h"
#include "InputNode.h"
#include "NeuronNode.h"

class NodePropertyDialog
{
private:
	NeuronNode * node;
	ComboBox cboFuncao;
	TextField editBias;
	bool visible;
	bool saved;
private:
	void salvar()
	{
		double bias;
		node->setFunctionType(cboFuncao.getSelected());
		// mantém o bias anterior se não for um número
		if(Node::parseNumber(editBias.getText(), bias))
			node->setBias(bias);
		saved = true;
		visible = false;
	}
public:
	NodePropertyDialog()
	{
		node = NULL;
		visible = false;
		saved = false;
		cboFuncao.add("Linear");
		cboFuncao.add("Degrau");
		cboFuncao.add("Bipolar");
		cboFuncao.add("Logística");
		cboFuncao.add("Hiperbólica");
	}
	void show(NeuronNode * node)
	{
		this->node = node;
		cboFuncao.close();
		cboFuncao.setSelected(node->getFunctionType());
		editBias.setText(Node::formatNumber(node->getBias(), 6));
		editBias.selectAll();
		visible = true;
	}
	bool isVisible()
	{
		return visible;
	}
	/**
	 * Retorna true uma única vez depois que o neurônio foi alterado
	 */
	bool takeSaved()
	{
		bool result = saved;
		saved = false;
		return result;
	}
	void update()
	{
		if(IsKeyPressed(KEY_ESCAPE))
		{
			if(!cboFuncao.close())
				visible = false;
		}
		// com a lista aberta o teclado escolhe a função
		else if(cboFuncao.isOpen())
			cboFuncao.update();
		else
		{
			bool enter = cboFuncao.update();
			if(editBias.update() || enter)
				salvar();
		}
	}
	void layout()
	{
		ui::dialog(CLAY_STRING("Informações do Neurônio"), 300, [&]()
		{
			ui::label(CLAY_STRING("Função de ativação"), 16);
			cboFuncao.layout(CLAY_ID("CboFuncao"), CLAY_ID("CboFuncaoLista"), 16);
			ui::label(CLAY_STRING("Bias"), 16);
			editBias.layout(CLAY_ID("EditBias"), 16);
			ui::dialogButtons([&]()
			{
				if(ui::button(CLAY_ID("NodeSalvar"), CLAY_STRING("Salvar"), true, 80))
					salvar();
				if(ui::button(CLAY_ID("NodeCancelar"), CLAY_STRING("Cancelar"), true, 80))
					visible = false;
			});
		});
	}
};

class InputPropertyDialog
{
private:
	InputNode * node;
	ComboBox cboTipo;
	bool visible;
	bool saved;
private:
	void salvar()
	{
		node->setBias(cboTipo.getSelected() == 1);
		saved = true;
		visible = false;
	}
public:
	InputPropertyDialog()
	{
		node = NULL;
		visible = false;
		saved = false;
		cboTipo.add("Dado (0 ou 1)");
		cboTipo.add("Bias (sempre 1)");
	}
	void show(InputNode * node)
	{
		this->node = node;
		cboTipo.close();
		cboTipo.setSelected(node->isBias() ? 1 : 0);
		visible = true;
	}
	bool isVisible()
	{
		return visible;
	}
	/**
	 * Retorna true uma única vez depois que a entrada foi alterada
	 */
	bool takeSaved()
	{
		bool result = saved;
		saved = false;
		return result;
	}
	void update()
	{
		if(IsKeyPressed(KEY_ESCAPE))
		{
			if(!cboTipo.close())
				visible = false;
		}
		else if(cboTipo.update())
			salvar();
	}
	void layout()
	{
		ui::dialog(CLAY_STRING("Informações da Entrada"), 300, [&]()
		{
			ui::label(CLAY_STRING("Tipo"), 16);
			cboTipo.layout(CLAY_ID("CboTipo"), CLAY_ID("CboTipoLista"), 16);
			ui::dialogButtons([&]()
			{
				if(ui::button(CLAY_ID("InputSalvar"), CLAY_STRING("Salvar"), true, 80))
					salvar();
				if(ui::button(CLAY_ID("InputCancelar"), CLAY_STRING("Cancelar"), true, 80))
					visible = false;
			});
		});
	}
};

#endif /* _NODEPROPERTIES_H_ */
