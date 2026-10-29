#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <filesystem>
#include <stdexcept>
#include "ClayRaylib.h"
#include "Input.h"
#include "Platform.h"
#include "Resources.h"
#include "Widgets.h"
#include "NeuralNetwork.h"
#include "EdgeProperties.h"
#include "NodeProperties.h"

class RedesNeuraisFrame
{
private:
	NeuralNetwork* neuralNetwork;
	CustomDraw neuralNetworkDraw;
	EdgePropertyDialog edgePropertyDialog;
	NodePropertyDialog nodePropertyDialog;
	InputPropertyDialog inputPropertyDialog;
	MessageDialog messageDialog;
	ConfirmDialog confirmDialog;
	// arquivo aberto, onde "Salvar" grava (UTF-8)
	std::string fileName;
	// arquivo solto na janela aguardando confirmação
	std::string pendingFile;
	std::string title;
	// resultado da última ação, mostrado na barra de status
	std::string message;
	std::string status;
	std::string errorText;
	Vector2 mouse;
	int minWidth;
private:
	static void paintNeuralNetwork(void* context, Rectangle bounds)
	{
		((NeuralNetwork*)context)->paint(bounds);
	}
	void changeNodeProperties(Node* node)
	{
		if(node->is(Node::TypeNeuron))
			nodePropertyDialog.show((NeuronNode*)node);
		else if(node->is(Node::TypeInput))
			inputPropertyDialog.show((InputNode*)node);
	}
	void changeEdgeProperties(Edge* edge)
	{
		edgePropertyDialog.show(edge);
	}
	/**
	 * Abre o diálogo do neurônio ou da conexão selecionada
	 */
	void changeSelected()
	{
		if(neuralNetwork->getSelectedNode() != NULL)
			changeNodeProperties(neuralNetwork->getSelectedNode());
		else if(neuralNetwork->getSelectedEdge() != NULL)
			changeEdgeProperties(neuralNetwork->getSelectedEdge());
	}
	void executar()
	{
		neuralNetwork->setRunning(!neuralNetwork->isRunning());
		message.clear();
	}
	void treinar()
	{
		Execution::TrainResult result = neuralNetwork->train();
		std::string errors = "erro de " + Node::formatNumber(result.errorBefore, 2) + " para " +
			Node::formatNumber(result.errorAfter, 2) + ".";
		switch(result.status)
		{
		case Execution::Trained:
			message = "Treinado em " + std::to_string(result.iterations) +
				(result.iterations == 1 ? " iteração: " : " iterações: ") + errors;
			break;
		case Execution::OnTarget:
			message = "As saídas já estão iguais aos alvos, os pesos foram mantidos.";
			break;
		case Execution::NotConverged:
			message = "Não convergiu em " + std::to_string(result.iterations) +
				" iterações: " + errors;
			break;
		case Execution::HasCycle:
			message = "A rede tem um ciclo e não pode ser treinada.";
			break;
		case Execution::NoOutput:
			message = "Adicione uma saída para treinar a rede.";
			break;
		}
	}
	void salvar()
	{
		try
		{
			neuralNetwork->saveToFile(fileName);
			message = "Rede salva em " + pathToUtf8(pathFromUtf8(fileName).filename()) + ".";
		}
		catch (const std::exception& e)
		{
			messageDialog.show("Erro", std::string("Falha ao salvar a rede\n") + e.what());
		}
	}
	void openFile(const std::string& path)
	{
		try
		{
			neuralNetwork->loadFromFile(path);
			fileName = path;
		}
		catch (const std::exception& e)
		{
			messageDialog.show("Erro", std::string("Falha na leitura da rede\n") + e.what());
		}
	}
	/**
	 * Abre o arquivo solto na janela, confirmando antes se há alterações
	 */
	void requestOpen(const std::string& path)
	{
		if(!neuralNetwork->isModified())
			return openFile(path);
		pendingFile = path;
		confirmDialog.show("Abrir rede", "A rede atual tem alterações não salvas que serão "
			"perdidas.\nAbrir " + pathToUtf8(pathFromUtf8(path).filename()) + "?", "Abrir");
	}
	bool isDialogVisible()
	{
		return messageDialog.isVisible() || confirmDialog.isVisible() ||
			edgePropertyDialog.isVisible() || nodePropertyDialog.isVisible() ||
			inputPropertyDialog.isVisible();
	}
	void dropFiles()
	{
		if(!IsFileDropped())
			return;
		FilePathList files = LoadDroppedFiles();
		std::string path = files.count > 0 ? files.paths[0] : "";
		UnloadDroppedFiles(files);
		// com um diálogo aberto o arquivo é ignorado
		if(!path.empty() && !isDialogVisible())
			requestOpen(path);
	}
	/**
	 * Encaminha teclado e mouse para o diálogo aberto ou para a área de edição
	 */
	void update()
	{
		Vector2 last = mouse;
		mouse = GetMousePosition();
		dropFiles();
		// diálogo modal: a área de edição não recebe nada neste quadro
		if(messageDialog.isVisible())
			return messageDialog.update();
		if(confirmDialog.isVisible())
			return confirmDialog.update();
		if(edgePropertyDialog.isVisible())
			return edgePropertyDialog.update();
		if(nodePropertyDialog.isVisible())
			return nodePropertyDialog.update();
		if(inputPropertyDialog.isVisible())
			return inputPropertyDialog.update();
		const int keys[] = { KEY_ESCAPE, KEY_DELETE, KEY_BACKSPACE, KEY_UP, KEY_DOWN, KEY_LEFT,
			KEY_RIGHT };
		for(int key : keys)
		{
			if(IsKeyPressed(key) || IsKeyPressedRepeat(key))
				neuralNetwork->keyDown(key);
		}
		Clay_ElementId id = CLAY_ID("NeuralNetwork");
		Clay_ElementData data = Clay_GetElementData(id);
		if(!data.found)
			return;
		int x = (int)(mouse.x - data.boundingBox.x);
		int y = (int)(mouse.y - data.boundingBox.y);
		bool over = Clay_PointerOver(id);
		bool moved = mouse.x != last.x || mouse.y != last.y;
		if(moved && (over || neuralNetwork->isMouseDown()))
			neuralNetwork->mouseMove(x, y);
		if(input::mousePressed() && over)
		{
			message.clear();
			neuralNetwork->mouseDown(x, y, MOUSE_BUTTON_LEFT);
		}
		if(input::mouseDoubleClicked() && over && neuralNetwork->mouseDoubleClick(x, y))
			changeSelected();
		if(input::mouseReleased() && !input::mouseDown())
			neuralNetwork->mouseUp(x, y, MOUSE_BUTTON_LEFT);
	}
	/**
	 * Aplica o resultado dos diálogos fechados neste quadro, retorna true se
	 * algo mudou
	 */
	bool updateDialogs()
	{
		bool changed = false;
		if(edgePropertyDialog.takeSaved() || nodePropertyDialog.takeSaved() ||
			inputPropertyDialog.takeSaved())
		{
			neuralNetwork->notifyDataChanged();
			message.clear();
			changed = true;
		}
		if(confirmDialog.takeConfirmed())
		{
			openFile(pendingFile);
			changed = true;
		}
		return changed;
	}
	/**
	 * Texto da barra de status: o resultado da última ação ou uma dica para o
	 * modo atual
	 */
	void updateStatus()
	{
		bool running = neuralNetwork->isRunning();
		errorText.clear();
		if(running && !neuralNetwork->hasCycle())
			errorText = "Erro: " + Node::formatNumber(neuralNetwork->getError(), 2);
		if(!message.empty())
			status = message;
		else if(running && neuralNetwork->hasCycle())
			status = "A rede tem um ciclo e não pode ser executada.";
		else if(running)
			status = "Clique nas entradas e nas saídas para alternar entre 0 e 1. Treinar ajusta "
				"os pesos para esta amostra.";
		else if(neuralNetwork->isAdding())
			status = "Clique na área de edição para posicionar. Esc cancela.";
		else if(neuralNetwork->isConnecting())
			status = "Clique em outro nó para conectar. Esc cancela.";
		else
			status = "Clique duplo em um neurônio, entrada ou conexão para alterar. Pesos e bias "
				"novos começam aleatórios.";
	}
	void layout()
	{
		updateStatus();
		CLAY({ .id = CLAY_ID("Frame"),
			.layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
				.padding = CLAY_PADDING_ALL(5), .childGap = 5,
				.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.backgroundColor = ui::WindowColor })
		{
			CLAY({ .id = CLAY_ID("NeuralNetwork"),
				.layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } },
				.custom = { .customData = &neuralNetworkDraw } }) {}
			CLAY({ .id = CLAY_ID("Buttons"),
				.layout = { .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
					.childGap = 5 } })
			{
				// a topologia fica fixa durante a execução
				bool running = neuralNetwork->isRunning();
				if(ui::button(CLAY_ID("AddNeuronio"), CLAY_STRING("Adicionar Neurônio"), !running))
					neuralNetwork->addNeuron();
				if(ui::button(CLAY_ID("AddEntrada"), CLAY_STRING("Adicionar Entrada"), !running))
					neuralNetwork->addInput();
				if(ui::button(CLAY_ID("AddSaida"), CLAY_STRING("Adicionar Saída"), !running))
					neuralNetwork->addOutput();
				bool selected = neuralNetwork->getSelectedNode() != NULL ||
					neuralNetwork->getSelectedEdge() != NULL;
				if(ui::button(CLAY_ID("Alterar"), CLAY_STRING("Alterar"), selected))
					changeSelected();
				ui::button(CLAY_ID("Configurar"), CLAY_STRING("Configurar dados"));
				if(ui::button(CLAY_ID("Executar"), running ? CLAY_STRING("Parar") :
					CLAY_STRING("Executar"), true, 80, running ? ui::ButtonPrimary :
					ui::ButtonDefault))
					executar();
				if(running && ui::button(CLAY_ID("Treinar"), CLAY_STRING("Treinar"), true, 0,
					ui::ButtonAccent))
					treinar();
				if(ui::button(CLAY_ID("Salvar"), CLAY_STRING("Salvar")))
					salvar();
			}
			CLAY({ .id = CLAY_ID("Status"),
				.layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(16) },
					.padding = { 2, 2, 0, 0 }, .childGap = 12,
					.childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER } } })
			{
				CLAY_TEXT(toClayString(status), ui::textConfig(12, ui::StatusTextColor));
				CLAY({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } } }) {}
				if(!errorText.empty())
					CLAY_TEXT(toClayString(errorText), ui::textConfig(12,
						neuralNetwork->isOnTarget() ? ui::SuccessColor : ui::WarningColor));
			}
		}
		if(messageDialog.isVisible())
			messageDialog.layout();
		else if(confirmDialog.isVisible())
			confirmDialog.layout();
		else if(edgePropertyDialog.isVisible())
			edgePropertyDialog.layout();
		else if(nodePropertyDialog.isVisible())
			nodePropertyDialog.layout();
		else if(inputPropertyDialog.isVisible())
			inputPropertyDialog.layout();
	}
	/**
	 * Mostra no título o arquivo aberto e um * se houver alterações
	 */
	void updateTitle()
	{
		std::string text = "Redes Neurais Artificiais - " +
			pathToUtf8(pathFromUtf8(fileName).filename());
		if(neuralNetwork->isModified())
			text += " *";
		if(text == title)
			return;
		title = text;
		SetWindowTitle(title.c_str());
	}
	/**
	 * Impede que a janela fique menor que a barra de botões, já contando o
	 * botão Treinar que aparece durante a execução
	 */
	void updateMinSize()
	{
		Clay_ElementData data = Clay_GetElementData(CLAY_ID("Buttons"));
		if(!data.found)
			return;
		float width = data.boundingBox.width + 10;
		if(!neuralNetwork->isRunning())
			width += MeasureTextEx(getFont(), "Treinar", ui::FontSize, 0).x + 24 + 5;
		if((int)ceilf(width) <= minWidth)
			return;
		minWidth = (int)ceilf(width);
		SetWindowMinSize(minWidth, 300);
		if(GetScreenWidth() < minWidth)
			SetWindowSize(minWidth, GetScreenHeight());
	}
public:
	RedesNeuraisFrame()
	{
		neuralNetwork = new NeuralNetwork();
		neuralNetworkDraw.draw = paintNeuralNetwork;
		neuralNetworkDraw.context = neuralNetwork;
		mouse = GetMousePosition();
		minWidth = 0;
		fileName = getDataFilePath();
		try
		{
			std::error_code error;
			if(std::filesystem::exists(pathFromUtf8(fileName), error))
				neuralNetwork->loadFromFile(fileName);
		}
		catch (const std::exception& e)
		{
			messageDialog.show("Aviso", std::string("Falha na leitura da rede\n") + e.what());
		}
	}
	~RedesNeuraisFrame()
	{
		delete neuralNetwork;
	}
	void run()
	{
		while(!WindowShouldClose())
		{
			input::beginFrame();
			Vector2 position = GetMousePosition();
			Clay_SetLayoutDimensions(Clay_Dimensions{ (float)GetScreenWidth(),
				(float)GetScreenHeight() });
			Clay_SetPointerState(Clay_Vector2{ position.x, position.y }, input::mouseDown());
			update();
			Clay_BeginLayout();
			layout();
			Clay_RenderCommandArray commands = Clay_EndLayout();
			ui::endFrame();
			bool changed = updateDialogs();
			// cliques são tratados durante o layout, então o que mudaram só aparece
			// no quadro seguinte: desenha mais um sem esperar por evento
			if(changed || input::mousePressed() || input::mouseReleased())
				DisableEventWaiting();
			else
				EnableEventWaiting();
			BeginDrawing();
			ClearBackground(RAYWHITE);
			ClayRaylib_Render(commands, getFonts());
			EndDrawing();
			updateTitle();
			updateMinSize();
		}
	}
};


int main()
{
	unsigned int flags = FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT;
#if defined(__APPLE__)
	// Retina; no Windows e no Linux a janela usa pixels físicos
	flags |= FLAG_WINDOW_HIGHDPI;
#endif
	SetConfigFlags(flags);
	SetTraceLogLevel(LOG_WARNING);
	InitWindow(800, 600, "Redes Neurais Artificiais");
	if(!IsWindowReady())
		return 0xFFFF;
	// Esc é usado para cancelar ações, não para fechar
	SetExitKey(KEY_NULL);
	// só redesenha quando chega algum evento
	EnableEventWaiting();
	input::install();
	int result = 0;
	try
	{
		loadResources();
		ClayRaylib_Initialize(getFonts());
		RedesNeuraisFrame f;
		f.run();
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "Error: %s\n", e.what());
		result = 0xFFFF;
	}
	ClayRaylib_Shutdown();
	unloadResources();
	CloseWindow();
	return result;
}
