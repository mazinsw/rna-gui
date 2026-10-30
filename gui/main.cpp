#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <filesystem>
#include <stdexcept>
#include "ClayRaylib.h"
#include "ContextMenu.h"
#include "InlineEditor.h"
#include "Input.h"
#include "Platform.h"
#include "Resources.h"
#include "Shapes.h"
#include "Widgets.h"
#include "NeuralNetwork.h"

// tecla que solta o nó do alinhamento ao mover
#if defined(__APPLE__)
#define FREE_MOVE_KEY "Option"
#else
#define FREE_MOVE_KEY "Alt"
#endif

class RedesNeuraisFrame
{
private:
	// nós antes desta posição ficariam embaixo do painel de botões
	static const int PanelRight = 200;
	NeuralNetwork* neuralNetwork;
	CustomDraw neuralNetworkDraw;
	CustomDraw legendDraw;
	InlineEditor editor;
	ContextMenu menu;
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
	float renderScale;
private:
	static void paintNeuralNetwork(void* context, Rectangle bounds)
	{
		((NeuralNetwork*)context)->paint(bounds);
	}
	/**
	 * Faixa de cores da legenda de influência
	 */
	static void paintLegend(void* context, Rectangle bounds)
	{
		const int steps = 24;
		for(int i = 0; i < steps; i++)
		{
			float left = bounds.x + bounds.width * i / steps;
			float right = bounds.x + bounds.width * (i + 1) / steps;
			DrawRectangleGradientH((int)left, (int)bounds.y, (int)ceilf(right - left),
				(int)bounds.height, Edge::influenceColor((float)i / steps),
				Edge::influenceColor((float)(i + 1) / steps));
		}
	}
	/**
	 * Canto da área de edição na janela
	 */
	Vector2 canvasOrigin()
	{
		Clay_ElementData data = Clay_GetElementData(CLAY_ID("NeuralNetwork"));
		if(!data.found)
			return Vector2{ 0, 0 };
		return Vector2{ data.boundingBox.x, data.boundingBox.y };
	}
	void executar()
	{
		editor.cancel();
		menu.close();
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
		case Execution::Stalled:
			message = "O erro parou de cair em " + std::to_string(result.iterations) +
				(result.iterations == 1 ? " iteração: " : " iterações: ") + errors +
				" Troque a função dos neurônios ou sorteie os pesos.";
			break;
		case Execution::Invalid:
			message = "A rede tem valores inválidos (nan ou inf) e não pode ser treinada. "
				"Sorteie os pesos para recomeçar.";
			break;
		case Execution::HasCycle:
			message = "A rede tem um ciclo e não pode ser treinada.";
			break;
		case Execution::NoOutput:
			message = "Adicione uma saída para treinar a rede.";
			break;
		}
	}
	/**
	 * Onde ficam os centros dos nós na área de edição: à direita do painel, com
	 * espaço para o bias acima, os valores abaixo e o status
	 */
	Rectangle editArea()
	{
		Clay_ElementData canvas = Clay_GetElementData(CLAY_ID("NeuralNetwork"));
		if(!canvas.found)
			return Rectangle{ 0, 0, 0, 0 };
		return Rectangle{ PanelRight + 40, 70, canvas.boundingBox.width - PanelRight - 100,
			canvas.boundingBox.height - 180 };
	}
	void sortearPesos()
	{
		editor.cancel();
		menu.close();
		neuralNetwork->randomizeWeights();
		message = "Pesos e bias sorteados entre -1 e 1.";
	}
	void reorganizar()
	{
		editor.cancel();
		menu.close();
		Rectangle area = editArea();
		if(area.width <= 0)
			return;
		int layers = neuralNetwork->arrange(area);
		neuralNetwork->fitView(area);
		if(layers == 0)
			message = "Adicione nós para reorganizar a rede.";
		else
			message = "Rede reorganizada em " + std::to_string(layers) +
				(layers == 1 ? " camada." : " camadas.");
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
		// o editor e o menu apontam para nós da rede que será substituída
		editor.cancel();
		menu.close();
		try
		{
			neuralNetwork->loadFromFile(path);
			neuralNetwork->keepRightOf(PanelRight);
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
		return messageDialog.isVisible() || confirmDialog.isVisible();
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
	 * Abre o editor no lugar ou o menu pedido pela área de edição
	 */
	void openRequest()
	{
		NeuralNetwork::Request request = neuralNetwork->takeRequest();
		Vector2 origin = canvasOrigin();
		if(request.kind == NeuralNetwork::Request::EditWeight)
			editor.show(request.edge);
		else if(request.kind == NeuralNetwork::Request::EditBias)
			editor.show((NeuronNode*)request.node);
		else if(request.kind == NeuralNetwork::Request::Menu && request.node != NULL)
		{
			// à direita do nó
			const Point& p = request.node->getPosition();
			Vector2 corner = neuralNetwork->toScreen(Vector2{ p.x + 36.0f, p.y - 28.0f });
			menu.show(neuralNetwork, request.node, NULL,
				Vector2{ origin.x + corner.x, origin.y + corner.y });
		}
		else if(request.kind == NeuralNetwork::Request::Menu)
		{
			Rectangle box = neuralNetwork->toScreen(request.edge->getLabelBox());
			menu.show(neuralNetwork, NULL, request.edge,
				Vector2{ origin.x + box.x + box.width + 6, origin.y + box.y });
		}
	}
	/**
	 * Encaminha teclado e mouse para o diálogo, o editor, o menu ou a área de
	 * edição
	 */
	void update()
	{
		Vector2 last = mouse;
		mouse = GetMousePosition();
		// antes dos diálogos e do menu, que tomam os eventos do quadro
		if(IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE))
			neuralNetwork->endPan();
		dropFiles();
		// diálogo modal: a área de edição não recebe nada neste quadro
		if(messageDialog.isVisible())
			return messageDialog.update();
		if(confirmDialog.isVisible())
			return confirmDialog.update();
		bool pressed = input::mousePressed() || input::mouseRightPressed();
		if(editor.isVisible())
		{
			// clique fora aplica o valor e segue para a área de edição, as teclas
			// ficam com o campo
			if(!pressed || Clay_PointerOver(CLAY_ID("Editor")))
			{
				if(editor.update())
					neuralNetwork->notifyDataChanged();
				return;
			}
			if(editor.commit())
				neuralNetwork->notifyDataChanged();
		}
		if(menu.isVisible())
		{
			// clique fora fecha o menu sem chegar à área de edição
			if(IsKeyPressed(KEY_ESCAPE) || (pressed && !Clay_PointerOver(CLAY_ID("Menu"))))
				menu.close();
			return;
		}
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
		// os painéis flutuantes capturam o mouse, então fora deles
		bool over = Clay_PointerOver(id);
		bool moved = mouse.x != last.x || mouse.y != last.y;
		// Alt muda o alinhamento do nó arrastado sem o mouse se mover
		bool alt = IsKeyPressed(KEY_LEFT_ALT) || IsKeyReleased(KEY_LEFT_ALT) ||
			IsKeyPressed(KEY_RIGHT_ALT) || IsKeyReleased(KEY_RIGHT_ALT);
		if((moved || alt) && (over || neuralNetwork->isMouseDown()))
			neuralNetwork->mouseMove(x, y);
		if(pressed && over)
			message.clear();
		// o botão do meio arrasta a vista mesmo sobre um nó, a roda dá zoom
		if(IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) && over)
			neuralNetwork->startPan(x, y);
		float wheel = GetMouseWheelMove();
		if(wheel != 0 && over)
			neuralNetwork->zoomAt(x, y, wheel);
		if(input::mousePressed() && over)
			neuralNetwork->mouseDown(x, y);
		if(input::mouseDoubleClicked() && over)
			neuralNetwork->mouseDoubleClick(x, y);
		if(input::mouseRightPressed() && over)
			neuralNetwork->mouseRightDown(x, y);
		if(input::mouseReleased() && !input::mouseDown())
			neuralNetwork->mouseUp(x, y);
		openRequest();
	}
	/**
	 * Aplica o resultado dos diálogos fechados neste quadro, retorna true se
	 * algo mudou
	 */
	bool updateDialogs()
	{
		if(!confirmDialog.takeConfirmed())
			return false;
		openFile(pendingFile);
		return true;
	}
	/**
	 * Texto da barra de status: o resultado da última ação ou uma dica para o
	 * que está selecionado
	 */
	void updateStatus()
	{
		bool running = neuralNetwork->isRunning();
		errorText.clear();
		if(running && !neuralNetwork->hasCycle())
			errorText = "Erro " + Node::formatNumber(neuralNetwork->getError(), 2);
		if(!message.empty())
			status = message;
		else if(editor.isVisible())
			status = "Digite o valor: Enter ou clique fora aplica, Esc cancela.";
		else if(menu.isVisible())
			status = "Escolha uma opção do menu. Esc fecha.";
		else if(running && neuralNetwork->hasCycle())
			status = "A rede tem um ciclo e não pode ser executada.";
		else if(neuralNetwork->isMoving())
			status = "Solte para posicionar. Segure " FREE_MOVE_KEY " para mover sem alinhar.";
		else if(neuralNetwork->isDrawingLayer())
			status = "Arraste para cima ou para baixo para mudar a quantidade. Solte para "
				"adicionar, Esc cancela.";
		else if(neuralNetwork->isConnecting())
			status = "Clique em outro nó para conectar. Esc cancela, Delete exclui o nó.";
		else if(neuralNetwork->getSelectedEdge() != NULL)
			status = running ? "Clique de novo no peso para editar." :
				"Clique de novo no peso para editar. Delete exclui a conexão.";
		else if(neuralNetwork->getSelectedBias() != NULL)
			status = "Clique de novo no bias para editar.";
		else if(running)
			status = "Clique nas entradas e nas saídas para alternar entre 0 e 1.";
		else if(neuralNetwork->isEmpty())
			status = "Segure Shift e arraste para baixo no fundo para criar as entradas.";
		else
			status = "Shift + arrastar no fundo cria uma camada. Clique direito no neurônio para "
				"trocar a função, clique no peso ou no bias para editar.";
	}
	/**
	 * Painel flutuante com os botões empilhados
	 */
	void toolbar()
	{
		bool running = neuralNetwork->isRunning();
		CLAY({ .id = CLAY_ID("Toolbar"),
			.layout = { .sizing = { CLAY_SIZING_FIXED(164), CLAY_SIZING_FIT(0) },
				.padding = CLAY_PADDING_ALL(8), .childGap = 2,
				.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.backgroundColor = ui::PanelColor,
			.cornerRadius = CLAY_CORNER_RADIUS(12),
			.floating = { .offset = { 12, 12 }, .zIndex = 2, .attachTo = CLAY_ATTACH_TO_PARENT },
			.border = { .color = ui::PanelBorderColor, .width = CLAY_BORDER_OUTSIDE(1) } })
		{
			// a topologia fica fixa durante a execução
			ui::groupTitle(CLAY_STRING("ADICIONAR"));
			if(ui::toolButton(CLAY_ID("AddNeuronio"), icons::Neuron, CLAY_STRING("Neurônio"),
				!running))
				neuralNetwork->addNeuron(editArea());
			if(ui::toolButton(CLAY_ID("AddEntrada"), icons::Input, CLAY_STRING("Entrada"),
				!running))
				neuralNetwork->addInput(editArea());
			if(ui::toolButton(CLAY_ID("AddSaida"), icons::Output, CLAY_STRING("Saída"), !running))
				neuralNetwork->addOutput(editArea());
			ui::separator(6);
			if(ui::toolButton(CLAY_ID("Executar"), running ? icons::Stop : icons::Play,
				running ? CLAY_STRING("Parar") : CLAY_STRING("Executar"), true,
				running ? ui::ButtonPrimary : ui::ButtonDefault))
				executar();
			if(running && ui::toolButton(CLAY_ID("Treinar"), icons::Train, CLAY_STRING("Treinar"),
				true, ui::ButtonAccent))
				treinar();
			if(ui::toolButton(CLAY_ID("SortearPesos"), icons::Dice, CLAY_STRING("Sortear pesos"),
				!neuralNetwork->isEmpty()))
				sortearPesos();
			ui::separator(6);
			if(ui::toolButton(CLAY_ID("Reorganizar"), icons::Arrange, CLAY_STRING("Reorganizar")))
				reorganizar();
			if(ui::toolButton(CLAY_ID("Salvar"), icons::Save, CLAY_STRING("Salvar")))
				salvar();
		}
	}
	/**
	 * Balão no canto inferior esquerdo com o status e o erro da execução,
	 * width é a largura da área de edição
	 */
	void statusBar(float width)
	{
		// deixa espaço para a legenda
		float maxWidth = width - 200;
		if(maxWidth < 220)
			maxWidth = 220;
		bool onTarget = neuralNetwork->isOnTarget();
		CLAY({ .id = CLAY_ID("Status"),
			.layout = { .sizing = { CLAY_SIZING_FIT(0, maxWidth), CLAY_SIZING_FIT(0) },
				.padding = { 12, 12, 6, 6 }, .childGap = 10,
				.childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER } },
			.backgroundColor = { 255, 255, 255, 242 },
			.cornerRadius = CLAY_CORNER_RADIUS(14),
			.floating = { .offset = { 12, -12 }, .zIndex = 2,
				.attachPoints = { .element = CLAY_ATTACH_POINT_LEFT_BOTTOM,
					.parent = CLAY_ATTACH_POINT_LEFT_BOTTOM },
				.attachTo = CLAY_ATTACH_TO_PARENT },
			.border = { .color = ui::PanelBorderColor, .width = CLAY_BORDER_OUTSIDE(1) } })
		{
			CLAY_TEXT(toClayString(status), ui::textConfig(ui::LabelFontSize, ui::StatusTextColor,
				CLAY_TEXT_WRAP_WORDS));
			if(!errorText.empty())
			{
				CLAY({ .layout = { .padding = { 8, 8, 1, 1 } },
					.backgroundColor = onTarget ? ui::SuccessLightColor : ui::WarningLightColor,
					.cornerRadius = CLAY_CORNER_RADIUS(9) })
				{
					CLAY_TEXT(toClayString(errorText), ui::textConfig(ui::LabelFontSize,
						onTarget ? ui::SuccessColor : ui::WarningColor));
				}
			}
		}
	}
	/**
	 * Escala de cores da influência no canto inferior direito
	 */
	void legend()
	{
		CLAY({ .id = CLAY_ID("Legend"),
			.layout = { .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
				.padding = { 10, 10, 8, 8 }, .childGap = 4,
				.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.backgroundColor = { 255, 255, 255, 242 },
			.cornerRadius = CLAY_CORNER_RADIUS(10),
			.floating = { .offset = { -12, -12 }, .zIndex = 2,
				.attachPoints = { .element = CLAY_ATTACH_POINT_RIGHT_BOTTOM,
					.parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM },
				.attachTo = CLAY_ATTACH_TO_PARENT },
			.border = { .color = ui::PanelBorderColor, .width = CLAY_BORDER_OUTSIDE(1) } })
		{
			CLAY_TEXT(CLAY_STRING("Influência na saída"), ui::textConfig(ui::CaptionFontSize,
				ui::StatusTextColor));
			CLAY({ .layout = { .sizing = { CLAY_SIZING_FIXED(130), CLAY_SIZING_FIXED(6) } },
				.custom = { .customData = &legendDraw } }) {}
			CLAY({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } } })
			{
				CLAY_TEXT(CLAY_STRING("baixa"), ui::textConfig(ui::CaptionFontSize, ui::MutedColor));
				CLAY({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } } }) {}
				CLAY_TEXT(CLAY_STRING("alta"), ui::textConfig(ui::CaptionFontSize, ui::MutedColor));
			}
		}
	}
	void layout()
	{
		updateStatus();
		// posição do quadro anterior, depois de declarado o elemento é recalculado
		Vector2 origin = canvasOrigin();
		Clay_ElementData canvas = Clay_GetElementData(CLAY_ID("NeuralNetwork"));
		float width = canvas.found ? canvas.boundingBox.width : (float)GetScreenWidth();
		CLAY({ .id = CLAY_ID("NeuralNetwork"),
			.layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } },
			.custom = { .customData = &neuralNetworkDraw } })
		{
			toolbar();
			statusBar(width);
			legend();
		}
		if(menu.isVisible())
			menu.layout();
		if(editor.isVisible())
			editor.layout(origin, neuralNetwork);
		if(messageDialog.isVisible())
			messageDialog.layout();
		else if(confirmDialog.isVisible())
			confirmDialog.layout();
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
public:
	RedesNeuraisFrame()
	{
		neuralNetwork = new NeuralNetwork();
		neuralNetworkDraw.draw = paintNeuralNetwork;
		neuralNetworkDraw.context = neuralNetwork;
		legendDraw.draw = paintLegend;
		legendDraw.context = NULL;
		mouse = GetMousePosition();
		renderScale = 0;
		fileName = getDataFilePath();
		try
		{
			std::error_code error;
			if(std::filesystem::exists(pathFromUtf8(fileName), error))
			{
				neuralNetwork->loadFromFile(fileName);
				neuralNetwork->keepRightOf(PanelRight);
			}
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
			shapes::beginFrame();
			// o Clay guarda a largura medida de cada texto, e ela muda com a fonte
			// de cada escala (janela levada a outra tela ou Retina detectado depois)
			if(getRenderScale() != renderScale)
			{
				renderScale = getRenderScale();
				Clay_ResetMeasureTextCache();
			}
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
			if(changed || input::mousePressed() || input::mouseReleased() ||
				input::mouseRightPressed())
				DisableEventWaiting();
			else
				EnableEventWaiting();
			BeginDrawing();
			ClearBackground(RAYWHITE);
			ClayRaylib_Render(commands);
			EndDrawing();
			updateTitle();
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
	// o painel de botões e o status precisam caber na área de edição
	SetWindowMinSize(560, 440);
	// Esc é usado para cancelar ações, não para fechar
	SetExitKey(KEY_NULL);
	// só redesenha quando chega algum evento
	EnableEventWaiting();
	input::install();
	int result = 0;
	try
	{
		loadResources();
		ClayRaylib_Initialize();
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
