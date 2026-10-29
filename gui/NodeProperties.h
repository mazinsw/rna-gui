#ifndef _NODEPROPERTIES_H_
#define _NODEPROPERTIES_H_
#include "Widgets.h"
#include "NeuronNode.h"

class NodePropertyDialog
{
private:
	NeuronNode * node;
	ComboBox cboFuncao;
	bool visible;
	bool saved;
private:
	void salvar()
	{
		node->setFunctionType(cboFuncao.getSelected());
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
		visible = true;
	}
	bool isVisible()
	{
		return visible;
	}
	/**
	 * Retorna true uma única vez depois que a função foi alterada
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
		else if(cboFuncao.update())
			salvar();
	}
	void layout()
	{
		ui::dialog(CLAY_STRING("Informações do Neurônio"), 300, [&]()
		{
			ui::label(CLAY_STRING("Função de ativação"), 16);
			cboFuncao.layout(CLAY_ID("CboFuncao"), CLAY_ID("CboFuncaoLista"), 16);
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

#endif /* _NODEPROPERTIES_H_ */
