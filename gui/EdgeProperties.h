#ifndef _EDGEPROPERTIES_H_
#define _EDGEPROPERTIES_H_
#include <algorithm>
#include <stdlib.h>
#include "Widgets.h"
#include "Edge.h"

class EdgePropertyDialog
{
private:
	Edge * edge;
	TextField editPeso;
	bool visible;
private:
	void salvar()
	{
		std::string str = editPeso.getText();
		std::replace(str.begin(), str.end(), ',', '.');
		char* end;
		double weight = strtod(str.c_str(), &end);
		// mantém o peso anterior se não for um número
		if(end != str.c_str())
			edge->setWeight(weight);
		visible = false;
	}
public:
	EdgePropertyDialog()
	{
		edge = NULL;
		visible = false;
	}
	void show(Edge * edge)
	{
		this->edge = edge;
		editPeso.setText(edge->getWeightString());
		editPeso.selectAll();
		visible = true;
	}
	bool isVisible()
	{
		return visible;
	}
	void update()
	{
		if(IsKeyPressed(KEY_ESCAPE))
			visible = false;
		else if(editPeso.update())
			salvar();
	}
	void layout()
	{
		ui::dialog(CLAY_STRING("Informações da Conexão"), 300, [&]()
		{
			ui::label(CLAY_STRING("Peso"), 16);
			editPeso.layout(CLAY_ID("EditPeso"), 16);
			ui::dialogButtons([&]()
			{
				if(ui::button(CLAY_ID("EdgeSalvar"), CLAY_STRING("Salvar"), true, 80))
					salvar();
				if(ui::button(CLAY_ID("EdgeCancelar"), CLAY_STRING("Cancelar"), true, 80))
					visible = false;
			});
		});
	}
};

#endif /* _EDGEPROPERTIES_H_ */
