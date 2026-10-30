#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <filesystem>
#include <stdexcept>
#include "ClayRaylib.h"
#include "ContextMenu.h"
#include "Draw.h"
#include "InlineEditor.h"
#include "Input.h"
#include "Number.h"
#include "Pixels.h"
#include "Platform.h"
#include "Resources.h"
#include "Settings.h"
#include "Shapes.h"
#include "Widgets.h"
#include "NeuralNetwork.h"

// tecla dos atalhos
#if defined(__APPLE__)
#define COMMAND_KEY "Cmd"
#else
#define COMMAND_KEY "Ctrl"
#endif

class RedesNeuraisFrame
{
private:
	// a rede fica à direita do painel de botões
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
	// a vista é ajustada à rede quando a área de edição tiver tamanho, a
	// restaurada só se não mostrar a rede
	bool fitPending;
	bool viewRestored;
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
				(int)bounds.height, draw::influenceColor((float)i / steps),
				draw::influenceColor((float)(i + 1) / steps));
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
	static std::string percent(double fraction)
	{
		return number::format(fraction * 100, 1) + "%";
	}
	void treinar()
	{
		editor.cancel();
		menu.close();
		if(neuralNetwork->isTraining())
		{
			neuralNetwork->stopTraining();
			return;
		}
		message.clear();
		neuralNetwork->train();
	}
	/**
	 * Mostra o resultado do treino que terminou
	 */
	void updateTraining()
	{
		Trainer::Result result;
		if(!neuralNetwork->takeResult(result))
			return;
		std::string errors = "erro de " + number::format(result.errorBefore, 2) + " para " +
			number::format(result.errorAfter, 2) + ".";
		std::string steps = std::to_string(result.steps) + (result.epochs ?
			(result.steps == 1 ? " época" : " épocas") :
			(result.steps == 1 ? " iteração" : " iterações"));
		std::string samples = " com " + samplesText(result.samples);
		std::string accuracy;
		if(result.testAccuracy >= 0)
			accuracy = ": acerto de " + percent(result.testAccuracy) + " no teste.";
		else if(result.accuracy >= 0)
			accuracy = ": acerto de " + percent(result.accuracy) + " no treino.";
		else
			accuracy = ".";
		switch(result.status)
		{
		case Trainer::Trained:
			message = "Treinado" + samples + " em " + steps + ": " + errors;
			break;
		case Trainer::OnTarget:
			message = result.samples == 1 ?
				"As saídas já estão iguais aos alvos, os pesos foram mantidos." :
				"As saídas já estão iguais aos alvos nas " + samplesText(result.samples) +
				", os pesos foram mantidos.";
			break;
		case Trainer::NotConverged:
			message = "Não convergiu em " + steps + samples + ": " + errors;
			break;
		case Trainer::Stalled:
			message = "O erro parou de cair em " + steps + samples + ": " + errors +
				" Troque a função das camadas ou sorteie os pesos.";
			break;
		case Trainer::Finished:
			message = "Treinado por " + steps + samples + accuracy;
			break;
		case Trainer::Stopped:
			message = result.epochs ? "Treino parado depois de " + steps + accuracy :
				"Treino parado em " + steps + ", a rede ficou com os melhores pesos: " + errors;
			break;
		case Trainer::Invalid:
			message = "A rede tem valores inválidos (nan ou inf) ou o treino divergiu. Sorteie os "
				"pesos para recomeçar.";
			break;
		case Trainer::NoOutput:
			message = "Adicione uma saída para treinar a rede.";
			break;
		case Trainer::NoSamples:
			message = "As saídas da softmax somam 1: clique no alvo da saída certa para treinar.";
			break;
		case Trainer::Mismatch:
		{
			const Network& network = neuralNetwork->getNetwork();
			message = "As imagens de " + neuralNetwork->getDataName() + " não servem para esta "
				"rede: ela precisa de " + std::to_string(network.inputs) + " entradas e " +
				std::to_string(network.outputs()) + " saídas.";
			break;
		}
		}
	}
	/**
	 * Onde fica a rede na área de edição: à direita do painel, com espaço para
	 * os rótulos e o status
	 */
	Rectangle editArea()
	{
		Clay_ElementData canvas = Clay_GetElementData(CLAY_ID("NeuralNetwork"));
		if(!canvas.found)
			return Rectangle{ 0, 0, 0, 0 };
		float scale = ClayRaylib_GetScale();
		float left = panelRight();
		return Rectangle{ left + 40, 110, canvas.boundingBox.width * scale - left - 100,
			canvas.boundingBox.height * scale - 220 };
	}
	void sortearPesos()
	{
		editor.cancel();
		menu.close();
		neuralNetwork->randomizeWeights();
		message = "Pesos e bias sorteados, menores nas camadas com mais ligações.";
	}
	void zerarAmostras()
	{
		bool data = !neuralNetwork->getDataName().empty();
		neuralNetwork->forgetSamples();
		message = data ? "Amostras e imagens esquecidas, Treinar volta a usar só a combinação "
			"da tela." : "Amostras apagadas, Treinar volta a usar só a combinação da tela.";
	}
	void ajustar()
	{
		Rectangle area = editArea();
		if(area.width > 0)
			neuralNetwork->fitView(area);
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
		editor.cancel();
		menu.close();
		try
		{
			neuralNetwork->loadFromFile(path);
			fileName = path;
			message.clear();
			ajustar();
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
			message = "Para receber a imagem " + name + " as entradas precisam ser uma imagem: "
				"clique direito nas entradas ou abra exemplos/digitos.bin.";
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
		if(neuralNetwork->showImage(std::vector<float>(values.begin(), values.end()), label))
			message = name + " reduzida a " + size + " nas entradas, alvo " +
				std::to_string(label) + ". Treinar a ensina.";
		else
			message = name + " reduzida a " + size + " nas entradas. Para ensinar, clique no "
				"alvo da saída certa e em Treinar.";
	}
	/**
	 * Lê as imagens soltas na janela para o treino: os arquivos do MNIST ou
	 * uma pasta de imagens
	 */
	void openData(const std::string& path)
	{
		editor.cancel();
		menu.close();
		try
		{
			int count = neuralNetwork->loadData(path);
			int tests = neuralNetwork->getTestCount();
			message = "Treinar usa as " + std::to_string(count) + " imagens de " +
				neuralNetwork->getDataName() + (tests > 0 ? ", com " + std::to_string(tests) +
				" de teste." : ".");
		}
		catch (const std::exception& e)
		{
			messageDialog.show("Erro", std::string("Falha na leitura das imagens\n") + e.what());
		}
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
		std::error_code error;
		std::filesystem::path file = pathFromUtf8(path);
		if(pixels::isImage(path))
			openImage(path);
		else if(Dataset::isIdx(file) || std::filesystem::is_directory(file, error))
			openData(path);
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
		if(request.kind == NeuralNetwork::Request::Edit)
			editor.show(request.box, neuralNetwork->getSelectedValue());
		else if(request.kind == NeuralNetwork::Request::Menu)
		{
			// à direita do clique
			Vector2 corner = neuralNetwork->toScreen(Vector2{ request.box.x, request.box.y });
			menu.show(neuralNetwork, request.layer, ClayRaylib_ToLayout(Vector2{
				origin.x + corner.x + 8, origin.y + corner.y - 8 }));
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
		// a vista gravada pode ser de outra disposição da rede
		Rectangle area = editArea();
		if(fitPending && area.width > 0)
		{
			if(!viewRestored || !neuralNetwork->isVisible(area))
				ajustar();
			fitPending = false;
		}
		updateTraining();
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
			float value;
			if(!pressed || Clay_PointerOver(CLAY_ID("Editor")))
			{
				if(editor.update(value))
					neuralNetwork->setSelectedValue(value);
				return;
			}
			if(editor.commit(value))
				neuralNetwork->setSelectedValue(value);
		}
		if(menu.isVisible())
		{
			// clique fora fecha o menu sem chegar à área de edição
			if(IsKeyPressed(KEY_ESCAPE) || (pressed && !Clay_PointerOver(CLAY_ID("Menu"))))
				menu.close();
			return;
		}
		const int keys[] = { KEY_ESCAPE, KEY_DELETE, KEY_BACKSPACE };
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
		if(moved && (over || neuralNetwork->isMouseDown()))
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
	 * Andamento do treino em andamento
	 */
	std::string progressText()
	{
		Trainer::Progress progress;
		if(!neuralNetwork->getProgress(progress))
			return "Treinando...";
		std::string text = "Treinando: ";
		if(progress.epochs)
		{
			text += "época " + std::to_string(progress.step) + " de " +
				std::to_string(progress.steps);
			if(progress.done < 1)
				text += " (" + std::to_string((int)(progress.done * 100)) + "%)";
			if(progress.accuracy >= 0)
				text += ", acerto de " + percent(progress.accuracy) + " no treino";
			if(progress.testAccuracy >= 0)
				text += " e " + percent(progress.testAccuracy) + " no teste";
		}
		else
			text += "iteração " + std::to_string(progress.step) + ", erro de " +
				number::format(progress.error, 2);
		return text + ". Parar interrompe o treino.";
	}
	/**
	 * Texto da barra de status: o resultado da última ação ou uma dica para o
	 * que está selecionado
	 */
	void updateStatus()
	{
		bool running = neuralNetwork->isRunning();
		NeuralNetwork::Part selected = neuralNetwork->getSelected();
		errorText.clear();
		resultText.clear();
		if(running && !neuralNetwork->getNetwork().layers.empty())
			errorText = "Erro " + number::format(neuralNetwork->getError(), 2);
		int recognized = neuralNetwork->isShowingImage() ? neuralNetwork->getRecognized() : -1;
		if(running && recognized >= 0)
			resultText = "Reconhecido " + std::to_string(recognized);
		if(neuralNetwork->isTraining())
			status = progressText();
		else if(!message.empty())
			status = message;
		else if(editor.isVisible())
			status = "Digite o valor: Enter ou clique fora aplica, Esc cancela.";
		else if(menu.isVisible())
			status = "Escolha uma opção do menu. Esc fecha.";
		else if(selected.kind == NeuralNetwork::Part::Weight)
			status = "Clique de novo no peso para editar.";
		else if(selected.kind == NeuralNetwork::Part::Bias)
			status = "Clique de novo no bias para editar.";
		else if(!neuralNetwork->getDataName().empty())
			status = "Treinar usa as " + std::to_string(neuralNetwork->getDataCount()) +
				" imagens de " + neuralNetwork->getDataName() + ". Zerar amostras volta às da tela.";
		else if(running && neuralNetwork->isShowingImage())
			status = "Arraste outra imagem para reconhecer. Clique no alvo da saída certa e em "
				"Treinar para ensinar esta.";
		else if(running && neuralNetwork->getImageSide() > 0)
			status = "Arraste a imagem de um dígito para a janela, ou a pasta do MNIST para "
				"treinar com ela.";
		else if(running)
		{
			int count = neuralNetwork->getSampleCount();
			status = "Clique nas entradas e nos alvos das saídas para alternar entre 0 e 1.";
			if(count == 0)
				status += " Cada combinação treinada fica guardada.";
			else if(count == 1)
				status += " Treinar usa também a amostra guardada.";
			else
				status += " Treinar usa também as " + samplesText(count) + " guardadas.";
		}
		else if(neuralNetwork->isEmpty())
			status = "Adicione as entradas e uma saída para começar a rede.";
		else if(selected.kind == NeuralNetwork::Part::Input)
			status = "Delete exclui a entrada. Clique direito nas entradas para formar uma imagem.";
		else if(selected.kind == NeuralNetwork::Part::Neuron)
			status = "Neurônio adiciona outro nesta camada. Clique direito troca a função da "
				"camada, Delete exclui o neurônio.";
		else if(selected.kind == NeuralNetwork::Part::Layer && selected.layer >= 0)
			status = "Neurônio adiciona nesta camada e Camada, depois dela. Clique direito troca a "
				"função, Delete exclui a camada.";
		else
			status = "Clique numa camada para adicionar nela, no peso ou no bias para editar. "
				"Clique direito troca a função da camada.";
	}
	/**
	 * Painel flutuante com os botões empilhados
	 */
	void toolbar()
	{
		bool running = neuralNetwork->isRunning();
		bool training = neuralNetwork->isTraining();
		// a estrutura fica fixa durante a execução e o treino
		bool editing = !running && !training;
		CLAY({ .id = CLAY_ID("Toolbar"),
			.layout = { .sizing = { CLAY_SIZING_FIXED(164), CLAY_SIZING_FIT(0) },
				.padding = CLAY_PADDING_ALL(8), .childGap = 2,
				.layoutDirection = CLAY_TOP_TO_BOTTOM },
			.backgroundColor = ui::PanelColor,
			.cornerRadius = CLAY_CORNER_RADIUS(12),
			.floating = { .offset = { 12, 12 }, .zIndex = 2, .attachTo = CLAY_ATTACH_TO_PARENT },
			.border = { .color = ui::PanelBorderColor, .width = CLAY_BORDER_OUTSIDE(1) } })
		{
			ui::groupTitle(CLAY_STRING("ADICIONAR"));
			if(ui::toolButton(CLAY_ID("AddEntrada"), icons::Input, CLAY_STRING("Entrada"), editing))
				neuralNetwork->addInput();
			if(ui::toolButton(CLAY_ID("AddNeuronio"), icons::Neuron, CLAY_STRING("Neurônio"),
				editing))
				neuralNetwork->addNeuron();
			if(ui::toolButton(CLAY_ID("AddCamada"), icons::Arrange, CLAY_STRING("Camada"), editing))
				neuralNetwork->addLayer();
			if(ui::toolButton(CLAY_ID("AddSaida"), icons::Output, CLAY_STRING("Saída"), editing))
				neuralNetwork->addOutput();
			ui::separator(6);
			if(ui::toolButton(CLAY_ID("Executar"), running ? icons::Stop : icons::Play,
				running ? CLAY_STRING("Parar") : CLAY_STRING("Executar"), !training,
				running ? ui::ButtonPrimary : ui::ButtonDefault))
				executar();
			bool data = !neuralNetwork->getDataName().empty();
			if(training && ui::toolButton(CLAY_ID("Treinar"), icons::Stop,
				CLAY_STRING("Parar treino"), true, ui::ButtonPrimary))
				treinar();
			else if(!training && (running || data) && ui::toolButton(CLAY_ID("Treinar"),
				icons::Train, CLAY_STRING("Treinar"), true, ui::ButtonAccent))
				treinar();
			if(ui::toolButton(CLAY_ID("SortearPesos"), icons::Dice, CLAY_STRING("Sortear pesos"),
				!training && !neuralNetwork->getNetwork().layers.empty()))
				sortearPesos();
			if((data || (running && neuralNetwork->getSampleCount() > 0)) &&
				ui::toolButton(CLAY_ID("ZerarAmostras"), icons::Forget, CLAY_STRING("Zerar amostras"),
				!training))
				zerarAmostras();
			ui::separator(6);
			if(ui::toolButton(CLAY_ID("Ajustar"), icons::Fit, CLAY_STRING("Ajustar à janela")))
				ajustar();
			if(ui::toolButton(CLAY_ID("Salvar"), icons::Save, CLAY_STRING("Salvar"), !training))
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
		fitPending = true;
		viewRestored = false;
		fileName = getDataFilePath();
		try
		{
			std::error_code error;
			if(std::filesystem::exists(pathFromUtf8(fileName), error))
			{
				neuralNetwork->loadFromFile(fileName);
				loadSamples(fileName);
				// a vista gravada é a desta rede se ela foi a última aberta
				if(settings.fileName == fileName)
				{
					neuralNetwork->setView(settings.zoom, settings.pan);
					viewRestored = true;
				}
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
			// no quadro seguinte: desenha mais um sem esperar por evento. Durante o
			// treino redesenha sempre, para mostrar o andamento
			if(changed || input::mousePressed() || input::mouseReleased() ||
				input::mouseRightPressed() || neuralNetwork->isTraining() || fitPending)
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
