#ifndef _NEURALNETWORK_H_
#define _NEURALNETWORK_H_
#include <raylib.h>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include "../training/Dataset.h"
#include "../training/Network.h"
#include "../training/Trainer.h"

/**
 * Área de edição da rede em camadas, recebe os eventos de mouse com
 * coordenadas relativas ao seu canto superior esquerdo. As entradas e as
 * camadas ficam em colunas, da esquerda para a direita: as pequenas mostram
 * os nós e as ligações, as grandes viram blocos com o mapa dos pesos e a
 * imagem grande, um quadro com os pixels. A vista tem zoom e deslocamento.
 */
class NeuralNetwork
{
public:
	/**
	 * Parte da rede apontada ou selecionada
	 */
	struct Part
	{
		enum Kind
		{
			None,
			Input,
			Neuron,
			// ligação da entrada (ou neurônio da camada anterior) from ao
			// neurônio to
			Weight,
			Bias,
			// a coluna inteira, das entradas com layer -1
			Layer,
			// alvo da saída to, na execução
			Target
		};
		Kind kind;
		// camada, -1 nas entradas
		int layer;
		int from, to;
	};

	/**
	 * Pedido para a janela: editar no lugar o peso ou o bias selecionado ou
	 * abrir o menu da camada (-1 nas entradas)
	 */
	struct Request
	{
		enum Kind
		{
			None,
			Edit,
			Menu
		};
		Kind kind;
		int layer;
		// rótulo editado ou o que recebe o menu, na rede
		Rectangle box;
	};
private:
	/**
	 * Coluna do desenho: as entradas ou uma camada
	 */
	struct Column
	{
		enum Kind
		{
			// nós em pé, os centros em x = left
			Nodes,
			// entradas de uma imagem pequena, um nó por pixel
			Grid,
			// camada ou entradas grandes demais para os nós
			Block,
			// imagem grande
			Frame
		};
		Kind kind;
		int count;
		float left, right, top, bottom;
		// as ligações com a anterior são desenhadas uma a uma
		bool lines;

		/**
		 * Centro do nó i, nas colunas Nodes e Grid
		 */
		Vector2 node(int i) const;
	};

	/**
	 * Rótulo ou caixa desenhada no último quadro, para os cliques
	 */
	struct Box
	{
		Rectangle box;
		Part part;
	};

	struct Job;
private:
	Network network;
	// amostras guardadas pelo treino
	Dataset samples;
	// imagens de fora, que o treino usa no lugar das amostras, e o teste
	Dataset data, test;
	std::string dataName;
	// valores das entradas e alvos das saídas na tela e os valores de cada
	// camada calculados com eles
	std::vector<float> inputs, targets;
	std::vector<std::vector<float> > values;
	// pesos e bias alterados pelo último treino
	std::vector<std::vector<char> > changedWeights, changedBias;
	std::vector<Column> columns;
	std::vector<Box> boxes;
	Part selected;
	bool running;
	// as entradas têm os pixels de uma imagem, cada saída é uma classe
	bool showingImage;
	bool modified;
	// vista: posição na área de edição = posição na rede * zoom + pan
	float zoom;
	Vector2 pan;
	// arrastando a vista, com a última posição do mouse na área de edição
	bool panning;
	int panX, panY;
	Request request;
	std::mt19937 random;
	// pixels das entradas no quadro e mapa dos pesos de cada camada em bloco,
	// refeitos quando mudam
	Texture2D image;
	bool imageDirty;
	std::vector<Texture2D> heatmaps;
	bool heatmapsDirty;
	std::unique_ptr<Job> job;
private:
	/**
	 * Monta as colunas da rede atual, centralizadas na altura 0
	 */
	void arrange();
	void draw();
	void drawLayerLines(int layer);
	void drawColumn(int column);
	void drawCaption(int column);
	void updateTextures();
	void unloadTextures();

	/**
	 * Indica se o bloco da camada mostra os pesos de cada neurônio na forma da
	 * imagem das entradas
	 */
	bool isMosaic(int layer);

	/**
	 * Parte sob o ponto da rede, os rótulos por cima dos nós e das ligações
	 */
	Part partAt(Vector2 point);
	bool isSelected(Part::Kind kind, int layer, int from, int to = 0);
	void select(Part part);

	/**
	 * Recalcula os valores de cada camada com as entradas da tela
	 */
	void execute();

	/**
	 * Depois de mudar a estrutura: ajusta as entradas e os alvos da tela e
	 * esquece as amostras se as entradas ou as saídas mudaram
	 */
	void structureChanged(int oldInputs, int oldOutputs);
	void clearChanged();
	Vector2 toWorld(int x, int y);

	/**
	 * Limites da rede desenhada, na rede
	 */
	Rectangle bounds();
	void startPanning(int x, int y);

	/**
	 * Para o treino em andamento e espera a thread terminar
	 */
	void cancelTraining();
public:
	NeuralNetwork();
	~NeuralNetwork();

	/**
	 * Desenha a área de edição nos limites informados
	 */
	void paint(Rectangle bounds);

	/**
	 * Botão esquerdo pressionado: seleciona a parte sob o mouse e, no fundo,
	 * arrasta a vista. Na execução alterna a entrada ou o alvo da saída. No
	 * peso, no bias ou no nome da camada o segundo clique pede a edição.
	 */
	void mouseDown(int x, int y);
	void mouseUp(int x, int y);
	void mouseMove(int x, int y);

	/**
	 * Clique duplo: pede a edição do peso ou do bias e o menu da camada
	 */
	void mouseDoubleClick(int x, int y);

	/**
	 * Botão direito: seleciona e pede o menu da camada
	 */
	void mouseRightDown(int x, int y);

	/**
	 * Delete exclui a entrada, o neurônio ou a camada selecionada, Esc tira a
	 * seleção
	 */
	void keyDown(int key);

	/**
	 * Indica se a vista está sendo arrastada
	 */
	bool isMouseDown();

	/**
	 * Zoom da roda do mouse, o ponto da rede sob o mouse fica parado
	 */
	void zoomAt(int x, int y, float wheel);

	/**
	 * Botão do meio pressionado: arrasta a vista mesmo sobre um nó
	 */
	void startPan(int x, int y);
	void endPan();

	/**
	 * Posição na área de edição de um ponto da rede
	 */
	Vector2 toScreen(Vector2 point);
	Rectangle toScreen(Rectangle rect);

	/**
	 * Centraliza a rede na área, com o zoom que a mostra inteira, sem passar
	 * de 100%
	 */
	void fitView(Rectangle area);

	/**
	 * Indica se alguma parte da rede aparece na área com a vista atual
	 */
	bool isVisible(Rectangle area);

	/**
	 * Zoom e deslocamento da vista, para voltar a ela ao abrir a rede de novo
	 */
	float getZoom();
	Vector2 getPan();
	void setView(float zoom, Vector2 pan);

	/**
	 * Uma entrada a mais; as entradas deixam de ser uma imagem
	 */
	void addInput();

	/**
	 * Um neurônio a mais na camada selecionada ou, sem ela, na última oculta;
	 * sem oculta cria uma
	 */
	void addNeuron();

	/**
	 * Camada oculta nova de 2 neurônios depois da selecionada ou antes da
	 * saída; sem camadas cria a de saída
	 */
	void addLayer();

	/**
	 * Um neurônio a mais na camada de saída, criada se não houver
	 */
	void addOutput();

	/**
	 * Exclui a entrada, o neurônio ou a camada selecionada
	 */
	void removeSelected();
	void removeLayer(int layer);
	void setFunction(int layer, int function);

	/**
	 * Troca as entradas por uma imagem side x side, 0 volta a entradas soltas
	 * na mesma quantidade
	 */
	void setImageSide(int side);

	/**
	 * Valor editado no lugar: o peso ou o bias selecionado, a quantidade de
	 * neurônios da camada selecionada ou, nas entradas, a quantidade delas ou
	 * o lado da imagem
	 */
	float getSelectedValue();

	/**
	 * Muda o valor editado no lugar, retorna true se mudou. Uma quantidade
	 * fora dos limites não muda nada.
	 */
	bool setSelectedValue(float value);

	/**
	 * Grava a rede e, ao lado, as amostras do treino (rede.amostras.txt); sem
	 * amostras apaga o arquivo delas. O caminho é UTF-8.
	 */
	void saveToFile(const std::string& fileName);

	/**
	 * Lê as amostras gravadas ao lado da rede, se o arquivo existir, e retorna
	 * quantas leu. Se ele não combina com a rede lança exceção e a rede fica
	 * sem amostras.
	 */
	int loadSamples(const std::string& fileName);

	/**
	 * Substitui a rede pela do arquivo, o caminho é UTF-8. Se o arquivo for
	 * inválido lança exceção e a rede atual não é alterada.
	 */
	void loadFromFile(const std::string& fileName);

	/**
	 * Lê as imagens que o treino passa a usar no lugar das amostras: os
	 * arquivos IDX do MNIST, com o teste ao lado, ou uma pasta de imagens com
	 * o dígito no começo do nome. Retorna quantas leu; lança exceção se
	 * falhar.
	 */
	int loadData(const std::string& path);

	/**
	 * Nome dos dados de fora, vazio sem eles
	 */
	const std::string& getDataName();
	int getDataCount();
	int getTestCount();

	/**
	 * Indica se há alterações desde a última leitura ou gravação
	 */
	bool isModified();
	Part getSelected();
	const Network& getNetwork();

	/**
	 * Retorna o pedido feito pelos eventos de mouse e o descarta
	 */
	Request takeRequest();

	/**
	 * Entra ou sai do modo de execução
	 */
	void setRunning(bool running);
	bool isRunning();

	/**
	 * Começa o treino numa thread, com os dados de fora ou com as amostras
	 * guardadas mais a combinação da tela (withScreen). O resultado vem de
	 * takeResult.
	 */
	void train(bool withScreen = true);
	bool isTraining();
	void stopTraining();

	/**
	 * Andamento do treino, retorna false antes do primeiro aviso
	 */
	bool getProgress(Trainer::Progress& progress);

	/**
	 * Se o treino terminou, põe a rede treinada no lugar, marca os pesos e os
	 * bias alterados e retorna true com o resultado
	 */
	bool takeResult(Trainer::Result& result);

	/**
	 * Sorteia de novo os pesos e os bias
	 */
	void randomizeWeights();

	/**
	 * Amostras guardadas pelo treino, todas usadas a cada treino
	 */
	int getSampleCount();

	/**
	 * Esquece as amostras e os dados de fora, o próximo treino usa só a
	 * combinação da tela
	 */
	void forgetSamples();
	bool isEmpty();
	double getError();
	bool isOnTarget();

	/**
	 * Lado do quadrado de pixels das entradas, 0 se elas não são uma imagem
	 */
	int getImageSide();

	/**
	 * Entra na execução com os pixels da imagem nas entradas. O alvo da saída
	 * do rótulo fica em 1 e os das outras em 0; sem rótulo, ou com um além das
	 * saídas, todos ficam em 0. Retorna true se o rótulo marcou um alvo.
	 */
	bool showImage(const std::vector<float>& values, int label);

	/**
	 * Indica se as entradas têm a imagem: a execução não foi parada e nenhuma
	 * entrada foi alternada desde showImage
	 */
	bool isShowingImage();

	/**
	 * Saída de maior valor, contando de 0, ou -1 sem saídas ou com valores
	 * inválidos
	 */
	int getRecognized();

	/**
	 * Saída com o único alvo diferente de 0, ou -1
	 */
	int getExpected();
};

#endif /* _NEURALNETWORK_H_ */
