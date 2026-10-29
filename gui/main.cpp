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
	MessageDialog messageDialog;
	ConfirmDialog confirmDialog;
	// arquivo aberto, onde "Salvar" grava (UTF-8)
	std::string fileName;
	// arquivo solto na janela aguardando confirmação
	std::string pendingFile;
	std::string title;
	Vector2 mouse;
	bool minSizeSet;
private:
	static void paintNeuralNetwork(void* context, Rectangle bounds)
	{
		((NeuralNetwork*)context)->paint(bounds);
	}
	void changeNodeProperties(Node* node)
	{
		if(!node->is(Node::TypeNeuron))
			return;
		nodePropertyDialog.show((NeuronNode*)node);
	}
	void changeEdgeProperties(Edge* edge)
	{
		edgePropertyDialog.show(edge);
	}
	void salvar()
	{
		try
		{
			neuralNetwork->saveToFile(fileName);
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
			edgePropertyDialog.isVisible() || nodePropertyDialog.isVisible();
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
			neuralNetwork->mouseDown(x, y, MOUSE_BUTTON_LEFT);
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
		if(edgePropertyDialog.takeSaved() || nodePropertyDialog.takeSaved())
		{
			neuralNetwork->notifyDataChanged();
			changed = true;
		}
		if(confirmDialog.takeConfirmed())
		{
			openFile(pendingFile);
			changed = true;
		}
		return changed;
	}
	void layout()
	{
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
				if(ui::button(CLAY_ID("AddNeuronio"), CLAY_STRING("Adicionar Neurônio")))
					neuralNetwork->addNeuron();
				if(ui::button(CLAY_ID("AddEntrada"), CLAY_STRING("Adicionar Entrada")))
					neuralNetwork->addInput();
				if(ui::button(CLAY_ID("AddSaida"), CLAY_STRING("Adicionar Saída")))
					neuralNetwork->addOutput();
				bool selected = neuralNetwork->getSelectedNode() != NULL ||
					neuralNetwork->getSelectedEdge() != NULL;
				if(ui::button(CLAY_ID("Alterar"), CLAY_STRING("Alterar"), selected))
				{
					if(neuralNetwork->getSelectedNode() != NULL)
						changeNodeProperties(neuralNetwork->getSelectedNode());
					else if(neuralNetwork->getSelectedEdge() != NULL)
						changeEdgeProperties(neuralNetwork->getSelectedEdge());
				}
				ui::button(CLAY_ID("Configurar"), CLAY_STRING("Configurar dados"));
				ui::button(CLAY_ID("Executar"), CLAY_STRING("Executar"));
				if(ui::button(CLAY_ID("Salvar"), CLAY_STRING("Salvar")))
					salvar();
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
	 * Impede que a janela fique menor que a barra de botões
	 */
	void updateMinSize()
	{
		Clay_ElementData data = Clay_GetElementData(CLAY_ID("Buttons"));
		if(minSizeSet || !data.found)
			return;
		int width = (int)ceilf(data.boundingBox.width) + 10;
		SetWindowMinSize(width, 300);
		if(GetScreenWidth() < width)
			SetWindowSize(width, GetScreenHeight());
		minSizeSet = true;
	}
public:
	RedesNeuraisFrame()
	{
		neuralNetwork = new NeuralNetwork();
		neuralNetworkDraw.draw = paintNeuralNetwork;
		neuralNetworkDraw.context = neuralNetwork;
		mouse = GetMousePosition();
		minSizeSet = false;
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
