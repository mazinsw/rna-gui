#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <filesystem>
#include <stdexcept>
#include "ClayRaylib.h"
#include "ContextMenu.h"
#include "InlineEditor.h"
#include "Input.h"
#include "Pixels.h"
#include "Platform.h"
#include "Resources.h"
#include "Settings.h"
#include "Shapes.h"
#include "Widgets.h"
#include "NeuralNetwork.h"

// tecla que solta o nó do alinhamento ao mover e a dos atalhos
#if defined(__APPLE__)
#define FREE_MOVE_KEY "Option"
#define COMMAND_KEY "Cmd"
#else
#define FREE_MOVE_KEY "Alt"
#define COMMAND_KEY "Ctrl"
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
	// estado da janela e da vista, gravado ao sair
	Settings& settings;
	// arquivo aberto, onde "Salvar" grava (UTF-8)
	std::string fileName;
	// arquivo solto na janela aguardando confirmação
	std::string pendingFile;
	std::string title;
	// resultado da última ação, mostrado na barra de status
	std::string message;
	std::string status;
	std::string errorText;
	// saída de maior valor com uma imagem nas entradas
	std::string resultText;
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
		float scale = ClayRaylib_GetScale();
		return Vector2{ data.boundingBox.x * scale, data.boundingBox.y * scale };
	}
	/**
	 * Fim do painel de botões na área de edição, que cresce com a interface
	 */
	int panelRight()
	{
		return (int)roundf(PanelRight * ClayRaylib_GetScale());
	}
	void executar()
	{
		editor.cancel();
		menu.close();
		neuralNetwork->setRunning(!neuralNetwork->isRunning());
		message.clear();
	}
	static std::string samplesText(int count)
	{
		return std::to_string(count) + (count == 1 ? " amostra" : " amostras");
	}
	void treinar()
	{
		Training::Result result = neuralNetwork->train();
		std::string errors = "erro de " + Node::formatNumber(result.errorBefore, 2) + " para " +
			Node::formatNumber(result.errorAfter, 2) + ".";
		std::string iterations = std::to_string(result.iterations) +
			(result.iterations == 1 ? " iteração" : " iterações");
		std::string samples = " com " + samplesText(result.samples);
		switch(result.status)
		{
		case Training::Trained:
			message = "Treinado" + samples + " em " + iterations + ": " + errors;
			break;
		case Training::OnTarget:
			message = result.samples == 1 ?
				"As saídas já estão iguais aos alvos, os pesos foram mantidos." :
				"As saídas já estão iguais aos alvos nas " + samplesText(result.samples) +
				", os pesos foram mantidos.";
			break;
		case Training::NotConverged:
			message = "Não convergiu em " + iterations + samples + ": " + errors;
			break;
		case Training::Stalled:
			message = "O erro parou de cair em " + iterations + samples + ": " + errors +
				" Troque a função dos neurônios ou sorteie os pesos.";
			break;
		case Training::Invalid:
			message = "A rede tem valores inválidos (nan ou inf) e não pode ser treinada. "
				"Sorteie os pesos para recomeçar.";
			break;
		case Training::HasCycle:
			message = "A rede tem um ciclo e não pode ser treinada.";
			break;
		case Training::NoOutput:
			message = "Adicione uma saída para treinar a rede.";
			break;
		case Training::NoTarget:
			message = "As saídas da softmax somam 1: clique no alvo da saída certa para treinar.";
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
		float scale = ClayRaylib_GetScale();
		float left = panelRight();
		return Rectangle{ left + 40, 70, canvas.boundingBox.width * scale - left - 100,
			canvas.boundingBox.height * scale - 180 };
	}
	void sortearPesos()
	{
		editor.cancel();
		menu.close();
		neuralNetwork->randomizeWeights();
		message = "Pesos e bias sorteados entre -1 e 1.";
	}
	void zerarAmostras()
	{
		neuralNetwork->forgetSamples();
		message = "Amostras apagadas, Treinar volta a usar só a combinação da tela.";
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
			int count = neuralNetwork->getSampleCount();
			message = (count > 0 ? "Rede e " + samplesText(count) + " salvas em " :
				std::string("Rede salva em ")) + pathToUtf8(pathFromUtf8(fileName).filename()) + ".";
		}
		catch (const std::exception& e)
		{
			messageDialog.show("Erro", std::string("Falha ao salvar a rede\n") + e.what());
		}
	}
	/**
	 * Lê as amostras gravadas ao lado da rede, uma falha só deixa a rede sem
	 * elas
	 */
	void loadSamples(const std::string& path)
	{
		try
		{
			int count = neuralNetwork->loadSamples(path);
			if(count > 0)
				message = "Treinar usa as " + samplesText(count) + " gravadas com a rede.";
		}
		catch (const std::exception& e)
		{
			message = std::string("As amostras não foram lidas: ") + e.what();
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
			neuralNetwork->keepRightOf(panelRight());
			// a rede que não cabe em 100% aparece inteira
			Rectangle area = editArea();
			if(area.width > 0 && !neuralNetwork->fitsIn(area))
				neuralNetwork->fitView(area);
			fileName = path;
			loadSamples(path);
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
	/**
	 * Põe a imagem solta na janela nas entradas e executa a rede. O dígito que
	 * começa o nome do arquivo marca o alvo da saída dele.
	 */
	void openImage(const std::string& path)
	{
		editor.cancel();
		menu.close();
		std::string name = pathToUtf8(pathFromUtf8(path).filename());
		int side = neuralNetwork->getImageSide();
		if(side == 0)
		{
			message = "Para receber a imagem " + name + " as entradas de dado precisam formar "
				"um quadrado de pixels, como 64 para 8 x 8. Abra exemplos/digitos.bin.";
			return;
		}
		std::vector<double> values;
		try
		{
			values = pixels::fromFile(path, side);
		}
		catch (const std::exception& e)
		{
			messageDialog.show("Erro", std::string("Falha na leitura da imagem\n") + e.what());
			return;
		}
		int label = pixels::labelOf(path);
		std::string size = std::to_string(side) + " x " + std::to_string(side);
		if(neuralNetwork->showImage(values, label))
			message = name + " reduzida a " + size + " nas entradas, alvo " +
				std::to_string(label) + ". Treinar a ensina.";
		else
			message = name + " reduzida a " + size + " nas entradas. Para ensinar, clique no "
				"alvo da saída certa e em Treinar.";
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
		if(path.empty() || isDialogVisible())
			return;
		if(pixels::isImage(path))
			openImage(path);
		else
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
				ClayRaylib_ToLayout(Vector2{ origin.x + corner.x, origin.y + corner.y }));
		}
		else if(request.kind == NeuralNetwork::Request::Menu)
		{
			Rectangle box = neuralNetwork->toScreen(request.edge->getLabelBox());
			menu.show(neuralNetwork, NULL, request.edge,
				ClayRaylib_ToLayout(Vector2{ origin.x + box.x + box.width + 6, origin.y + box.y }));
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
		if(!Clay_GetElementData(id).found)
			return;
		Vector2 origin = canvasOrigin();
		int x = (int)(mouse.x - origin.x);
		int y = (int)(mouse.y - origin.y);
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
	 * Muda o tamanho da interface e o mostra no status, retorna true se mudou
	 */
	bool setScale(float scale)
	{
		if(scale == 1)
			message = "Interface no tamanho padrão, 100%.";
		else
			message = "Interface em " + std::to_string((int)roundf(scale * 100)) + "%" +
				(scale == ui::MaxScale ? ", o maior tamanho" : scale == ui::MinScale ?
				", o menor tamanho" : "") + ". " COMMAND_KEY "+0 volta ao padrão.";
		if(scale == ClayRaylib_GetScale())
			return false;
		// o menu ficaria onde foi aberto, longe do nó
		menu.close();
		ClayRaylib_SetScale(scale);
		return true;
	}
	/**
	 * Ctrl + e Ctrl - (Cmd no Mac) mudam o tamanho da interface e Ctrl 0 volta
	 * ao padrão, retorna true se mudou
	 */
	bool updateScale()
	{
		if(!ui::commandDown())
			return false;
		float scale = ClayRaylib_GetScale();
		if(IsKeyPressed(KEY_EQUAL) || IsKeyPressedRepeat(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD) ||
			IsKeyPressedRepeat(KEY_KP_ADD))
			return setScale(ui::nextScale(scale, 1));
		if(IsKeyPressed(KEY_MINUS) || IsKeyPressedRepeat(KEY_MINUS) ||
			IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressedRepeat(KEY_KP_SUBTRACT))
			return setScale(ui::nextScale(scale, -1));
		if(IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0))
			return setScale(1);
		return false;
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
		resultText.clear();
		if(running && !neuralNetwork->hasCycle())
			errorText = "Erro " + Node::formatNumber(neuralNetwork->getError(), 2);
		int recognized = neuralNetwork->isShowingImage() ? neuralNetwork->getRecognized() : -1;
		if(running && recognized >= 0)
			resultText = "Reconhecido " + std::to_string(recognized);
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
		else if(running && neuralNetwork->isShowingImage())
			status = "Arraste outra imagem para reconhecer. Clique no alvo da saída certa e em "
				"Treinar para ensinar esta.";
		else if(running && neuralNetwork->getImageSide() > 0)
			status = "Arraste a imagem de um dígito para a janela, ou clique nas entradas e nas "
				"saídas para alternar entre 0 e 1.";
		else if(running)
		{
			int count = neuralNetwork->getSampleCount();
			status = "Clique nas entradas e nas saídas para alternar entre 0 e 1.";
			if(count == 0)
				status += " Cada combinação treinada fica guardada.";
			else if(count == 1)
				status += " Treinar usa também a amostra guardada.";
			else
				status += " Treinar usa também as " + samplesText(count) + " guardadas.";
		}
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
			if(running && neuralNetwork->getSampleCount() > 0 &&
				ui::toolButton(CLAY_ID("ZerarAmostras"), icons::Forget, CLAY_STRING("Zerar amostras")))
				zerarAmostras();
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
			if(!resultText.empty())
			{
				// verde se é a saída do alvo, laranja se não, azul sem alvo
				int expected = neuralNetwork->getExpected();
				bool right = expected == neuralNetwork->getRecognized();
				CLAY({ .layout = { .padding = { 8, 8, 1, 1 } },
					.backgroundColor = expected < 0 ? ui::AccentLightHoverColor :
						right ? ui::SuccessLightColor : ui::WarningLightColor,
					.cornerRadius = CLAY_CORNER_RADIUS(9) })
				{
					CLAY_TEXT(toClayString(resultText), ui::textConfig(ui::LabelFontSize,
						expected < 0 ? ui::AccentDarkColor : right ? ui::SuccessColor :
						ui::WarningColor));
				}
			}
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
		float width = canvas.found ? canvas.boundingBox.width :
			ClayRaylib_GetLayoutDimensions().width;
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
	RedesNeuraisFrame(Settings& settings): settings(settings)
	{
		ClayRaylib_SetScale(settings.uiScale);
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
				neuralNetwork->keepRightOf(panelRight());
				loadSamples(fileName);
				// a vista gravada é a desta rede se ela foi a última aberta
				if(settings.fileName == fileName)
					neuralNetwork->setView(settings.zoom, settings.pan);
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
			// antes do layout, que é feito na escala nova
			bool scaled = updateScale();
			Vector2 position = ClayRaylib_ToLayout(GetMousePosition());
			Clay_SetLayoutDimensions(ClayRaylib_GetLayoutDimensions());
			Clay_SetPointerState(Clay_Vector2{ position.x, position.y }, input::mouseDown());
			update();
			Clay_BeginLayout();
			layout();
			Clay_RenderCommandArray commands = Clay_EndLayout();
			ui::endFrame();
			// o mouse sobre os controles só é conferido na escala nova no quadro
			// seguinte
			bool changed = updateDialogs() || scaled;
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
			captureWindow(settings);
		}
		settings.uiScale = ClayRaylib_GetScale();
		settings.fileName = fileName;
		settings.zoom = neuralNetwork->getZoom();
		settings.pan = neuralNetwork->getPan();
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
	Settings settings = loadSettings();
	InitWindow(settings.width, settings.height, "Redes Neurais Artificiais");
	if(!IsWindowReady())
		return 0xFFFF;
	SetWindowMinSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
	restoreWindow(settings);
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
		RedesNeuraisFrame f(settings);
		f.run();
		saveSettings(settings);
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
