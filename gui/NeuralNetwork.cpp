#include "NeuralNetwork.h"
#include "Draw.h"
#include "Number.h"
#include "Pixels.h"
#include "Platform.h"
#include "Shapes.h"
#include "Widgets.h"
#include <rlgl.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <stdexcept>
#include <thread>
// pontos da grade de fundo a cada GRID_SIZE pixels, o primeiro em GRID_OFFSET
#define GRID_SIZE 20
#define GRID_OFFSET 10
// limites do zoom e passo de cada volta da roda do mouse
#define MIN_ZOOM 0.1f
#define MAX_ZOOM 4.0f
#define ZOOM_STEP 1.1f
// menor distância na tela entre os pontos da grade
#define MIN_GRID_SPACING 12
// distância entre os nós de uma coluna, entre os pixels de uma imagem e entre
// as colunas
#define NODE_SPACING 120
#define PIXEL_SPACING 40
#define COLUMN_GAP 200
// saídas à direita dos neurônios da última camada
#define OUTPUT_GAP 140
#define BLOCK_WIDTH 160
#define BLOCK_HEIGHT 240
#define FRAME_SIZE 224
// acima disto a camada vira bloco e a imagem vira quadro
#define MAX_NODES 32
#define MAX_GRID_SIDE 16
// por camada, ligações desenhadas uma a uma e com o rótulo do peso
#define MAX_LINES 4000
#define MAX_WEIGHT_LABELS 50
// limites da quantidade de neurônios ou de entradas e do lado da imagem
// digitados no lugar
#define MAX_LAYER_SIZE 4096
#define MAX_IMAGE_SIDE 64
// quanto o nome da coluna cresce com o zoom afastado
#define MAX_CAPTION_SCALE 2.5f
// rótulo do peso nesta fração do caminho da entrada ao neurônio
#define LABEL_POSITION 0.4f

static const char *functionNames[] = { "Linear", "Degrau", "Bipolar", "Logística", "Hiperbólica",
	"ReLU", "Softmax" };

/**
 * Treino em andamento numa thread, com uma cópia da rede
 */
struct NeuralNetwork::Job
{
	Trainer trainer;
	Network network;
	// cópia das amostras guardadas; os dados de fora não mudam durante o treino
	Dataset samples;
	const Dataset* data;
	const Dataset* test;
	Trainer::Result result;
	std::mutex mutex;
	Trainer::Progress progress;
	bool hasProgress;
	std::atomic<bool> finished;
	std::thread thread;

	Job(): data(nullptr), test(nullptr), hasProgress(false), finished(false) {}
};

Vector2 NeuralNetwork::Column::node(int i) const
{
	if(kind == Grid)
	{
		int side = Network::imageSide(count);
		return Vector2{ left + PIXEL_SPACING * (i % side), top + PIXEL_SPACING * (i / side) };
	}
	return Vector2{ left, top + NODE_SPACING * i };
}

NeuralNetwork::NeuralNetwork(): random(std::random_device()())
{
	selected = Part{ Part::None, 0, 0, 0 };
	running = false;
	showingImage = false;
	modified = false;
	zoom = 1;
	pan = Vector2{ 0, 0 };
	panning = false;
	panX = panY = 0;
	request = Request{ Request::None, 0, Rectangle{ 0, 0, 0, 0 } };
	image = Texture2D{ 0, 0, 0, 0, 0 };
	imageDirty = true;
	heatmapsDirty = true;
}

NeuralNetwork::~NeuralNetwork()
{
	cancelTraining();
	unloadTextures();
}

void NeuralNetwork::arrange()
{
	columns.clear();
	// os blocos e o quadro crescem com a coluna de nós mais alta, para não
	// sumirem ao lado dela
	int tallest = 0;
	for(const Network::Layer& layer : network.layers)
		tallest = layer.size <= MAX_NODES ? std::max(tallest, layer.size) : tallest;
	float scale = std::max(1.0f, NODE_SPACING * (tallest - 1) * 0.6f / BLOCK_HEIGHT);
	float x = 0;
	for(int c = 0; c <= (int)network.layers.size(); c++)
	{
		Column column;
		column.count = c == 0 ? network.inputs : network.layers[c - 1].size;
		int side = c == 0 ? network.side : 0;
		float width = 0, height = 0;
		if(side > MAX_GRID_SIDE)
		{
			column.kind = Column::Frame;
			width = height = FRAME_SIZE * scale;
		}
		else if(side > 0)
		{
			column.kind = Column::Grid;
			width = height = PIXEL_SPACING * (side - 1);
		}
		else if(column.count > MAX_NODES)
		{
			// o mosaico dos pesos de uma imagem é quadrado
			column.kind = Column::Block;
			height = BLOCK_HEIGHT * scale;
			width = isMosaic(c - 1) ? height : BLOCK_WIDTH * scale;
		}
		else
		{
			column.kind = Column::Nodes;
			height = NODE_SPACING * std::max(0, column.count - 1);
		}
		column.left = x;
		column.right = x + width;
		column.top = -height / 2;
		column.bottom = height / 2;
		column.lines = false;
		if(c > 0)
		{
			const Column& previous = columns.back();
			column.lines = (previous.kind == Column::Nodes || previous.kind == Column::Grid) &&
				column.kind == Column::Nodes && previous.count * column.count <= MAX_LINES;
		}
		columns.push_back(column);
		x = column.right + COLUMN_GAP;
	}
}

void NeuralNetwork::paint(Rectangle bounds)
{
	DrawRectangleRec(bounds, ui::toColor(ui::CanvasColor));
	BeginScissorMode((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height);
	// grade de pontos. Com o zoom afastado fica um a cada 2, 4..., para os
	// pontos não se juntarem
	Color grid = ui::toColor(ui::GridColor);
	float spacing = GRID_SIZE * zoom;
	while(spacing < MIN_GRID_SPACING)
		spacing *= 2;
	Vector2 first = toScreen(Vector2{ GRID_OFFSET, GRID_OFFSET });
	float left = bounds.x + first.x - floorf(first.x / spacing) * spacing;
	float top = bounds.y + first.y - floorf(first.y / spacing) * spacing;
	for(float gy = top; gy < bounds.y + bounds.height; gy += spacing)
	{
		for(float gx = left; gx < bounds.x + bounds.width; gx += spacing)
			DrawRectangleRec(Rectangle{ gx - 0.75f, gy - 0.75f, 1.5f, 1.5f }, grid);
	}
	// desenha a rede com o zoom e o deslocamento da vista. Sem BeginMode2D: no
	// raylib 6 ele descarta a escala da tela Retina e a rede sairia com metade
	// do tamanho, longe de onde o mouse a encontra
	Vector2 offset = { bounds.x + pan.x, bounds.y + pan.y };
	shapes::setZoom(zoom);
	draw::setView(zoom, offset);
	rlPushMatrix();
	rlTranslatef(offset.x, offset.y, 0);
	rlScalef(zoom, zoom, 1);
	draw();
	rlPopMatrix();
	draw::setView(1, Vector2{ 0, 0 });
	shapes::setZoom(1);
	EndScissorMode();
}

void NeuralNetwork::draw()
{
	arrange();
	updateTextures();
	boxes.clear();
	for(size_t l = 0; l < network.layers.size(); l++)
		drawLayerLines((int)l);
	for(size_t c = 0; c < columns.size(); c++)
		drawColumn((int)c);
	for(size_t c = 0; c < columns.size(); c++)
		drawCaption((int)c);
}

void NeuralNetwork::drawLayerLines(int index)
{
	const Column& from = columns[index];
	const Column& to = columns[index + 1];
	const Network::Layer& layer = network.layers[index];
	if(from.count == 0)
		return;
	if(!to.lines)
	{
		// faixa entre as colunas, as ligações são muitas para desenhar
		float pad = from.kind == Column::Nodes ? draw::NeuronRadius : 0;
		float toPad = to.kind == Column::Nodes ? draw::NeuronRadius : 0;
		Color color = ui::toColor(ui::PillBorderColor);
		color.a = 110;
		Vector2 band[] = { { from.right + pad, from.top - pad }, { to.left - toPad, to.top - toPad },
			{ to.left - toPad, to.bottom + toPad }, { from.right + pad, from.bottom + pad } };
		shapes::fillPolygon(band, 4, color);
		return;
	}
	// influência na saída: na edição pelo peso, na execução pelo sinal que
	// passa na ligação
	int k = from.count, size = to.count;
	std::vector<float> influence(layer.weights.size());
	float highest = 0;
	for(int i = 0; i < k; i++)
	{
		float source = !running ? 1 : index == 0 ? inputs[i] : values[index - 1][i];
		for(int j = 0; j < size; j++)
		{
			float value = fabsf(layer.weights[(size_t)i * size + j] * source);
			value = std::isfinite(value) ? value : 0;
			influence[(size_t)i * size + j] = value;
			highest = std::max(highest, value);
		}
	}
	bool grid = from.kind == Column::Grid;
	for(int i = 0; i < k; i++)
	{
		for(int j = 0; j < size; j++)
		{
			float value = highest > 0 ? influence[(size_t)i * size + j] / highest : 0;
			float thickness = (grid ? 1 : 1.5f) + 3 * value;
			Color color = draw::influenceColor(value);
			if(isSelected(Part::Weight, index, i, j))
			{
				color = ui::toColor(ui::AccentColor);
				thickness = std::max(thickness, 2.5f);
			}
			shapes::line(from.node(i), to.node(j), thickness, color);
		}
	}
	// rótulos dos pesos por cima das linhas, o selecionado sempre
	bool labels = !grid && k * size <= MAX_WEIGHT_LABELS;
	for(int i = 0; i < k; i++)
	{
		for(int j = 0; j < size; j++)
		{
			bool selectedWeight = isSelected(Part::Weight, index, i, j);
			if(!labels && !selectedWeight)
				continue;
			// antes do meio, onde as ligações que se cruzam se encontram
			Vector2 a = from.node(i), b = to.node(j);
			bool changed = index < (int)changedWeights.size() &&
				changedWeights[index][(size_t)i * size + j];
			Rectangle box = draw::label(Vector2{ a.x + (b.x - a.x) * LABEL_POSITION,
				a.y + (b.y - a.y) * LABEL_POSITION }, "",
				layer.weights[(size_t)i * size + j], changed, selectedWeight);
			boxes.push_back(Box{ box, Part{ Part::Weight, index, i, j } });
		}
	}
}

void NeuralNetwork::drawColumn(int c)
{
	const Column& column = columns[c];
	int layer = c - 1;
	bool columnSelected = isSelected(Part::Layer, layer, 0);
	Rectangle bounds = { column.left, column.top, column.right - column.left,
		column.bottom - column.top };
	Color ring = ui::toColor(ui::SelectionRingColor);
	if(column.kind == Column::Block || column.kind == Column::Frame)
	{
		// caixa com o mapa dos pesos, as entradas grandes ou a imagem
		if(columnSelected)
			shapes::strokeRoundedRect(Rectangle{ bounds.x - 5, bounds.y - 5, bounds.width + 10,
				bounds.height + 10 }, 16, 4, ring);
		shapes::fillRoundedRect(bounds, 12, WHITE);
		shapes::strokeRoundedRect(bounds, 12, 2, ui::toColor(ui::NodeColor));
		Rectangle inner = { bounds.x + 10, bounds.y + 10, bounds.width - 20, bounds.height - 20 };
		Texture2D texture = c == 0 ? image : layer < (int)heatmaps.size() ? heatmaps[layer] :
			Texture2D{ 0, 0, 0, 0, 0 };
		if(texture.id != 0)
			DrawTexturePro(texture, Rectangle{ 0, 0, (float)texture.width, (float)texture.height },
				inner, Vector2{ 0, 0 }, 0, WHITE);
		if(c == 0 && column.kind == Column::Frame && std::all_of(inputs.begin(), inputs.end(),
			[](float value) { return value == 0; }))
			draw::textCentered("Arraste uma imagem", Vector2{ bounds.x + bounds.width / 2,
				bounds.y + bounds.height / 2 }, ui::LabelFontSize * std::min(MAX_CAPTION_SCALE, 1 / zoom),
				ui::toColor(ui::FaintColor));
		else if(texture.id == 0)
			draw::textCentered(std::to_string(column.count) + " entradas", Vector2{
				bounds.x + bounds.width / 2, bounds.y + bounds.height / 2 }, ui::LabelFontSize,
				ui::toColor(ui::MutedColor));
		boxes.push_back(Box{ bounds, Part{ Part::Layer, layer, 0, 0 } });
		return;
	}
	if(c == 0)
	{
		for(int i = 0; i < column.count; i++)
		{
			Vector2 center = column.node(i);
			draw::input(center, inputs[i], running, columnSelected ||
				isSelected(Part::Input, -1, i));
			boxes.push_back(Box{ Rectangle{ center.x - draw::InputHalf, center.y - draw::InputHalf,
				draw::InputHalf * 2, draw::InputHalf * 2 }, Part{ Part::Input, -1, i, i } });
		}
		return;
	}
	const Network::Layer& weights = network.layers[layer];
	bool last = layer + 1 == (int)network.layers.size();
	int recognized = running && showingImage ? getRecognized() : -1;
	for(int j = 0; j < column.count; j++)
	{
		Vector2 center = column.node(j);
		float r = draw::NeuronRadius;
		if(last)
		{
			// a saída repassa o valor do neurônio
			Vector2 output = { center.x + OUTPUT_GAP, center.y };
			shapes::line(center, output, 2, ui::toColor(ui::PillBorderColor));
			Rectangle target = draw::output(output, values.back()[j], targets[j], running,
				isSelected(Part::Neuron, layer, j));
			if(target.width > 0)
				boxes.push_back(Box{ target, Part{ Part::Target, layer, j, j } });
			if(running && showingImage)
			{
				// número da classe, a reconhecida em destaque
				std::string text = std::to_string(j);
				float width = draw::measureText(text, ui::LabelFontSize).x + 16;
				Vector2 middle = { output.x + 26 + width / 2, output.y };
				Color color = ui::toColor(ui::MutedColor);
				if(j == recognized)
				{
					draw::pill(Rectangle{ middle.x - width / 2, middle.y - 12, width, 24 },
						ui::toColor(ui::AccentColor), ui::toColor(ui::AccentColor));
					color = WHITE;
				}
				draw::textCentered(text, middle, ui::LabelFontSize, color);
			}
		}
		draw::neuron(center, weights.function, columnSelected || isSelected(Part::Neuron, layer, j));
		boxes.push_back(Box{ Rectangle{ center.x - r, center.y - r, 2 * r, 2 * r },
			Part{ Part::Neuron, layer, j, j } });
		bool changed = layer < (int)changedBias.size() && changedBias[layer][j];
		Rectangle bias = draw::label(Vector2{ center.x, center.y - 45 }, "b ", weights.bias[j],
			changed, isSelected(Part::Bias, layer, j));
		boxes.push_back(Box{ bias, Part{ Part::Bias, layer, j, j } });
		if(running)
			draw::neuronValue(center, values[layer][j]);
	}
}

void NeuralNetwork::drawCaption(int c)
{
	const Column& column = columns[c];
	int layer = c - 1;
	std::string text;
	if(c == 0)
	{
		if(network.inputs == 0)
			return;
		text = network.side > 0 ? "Imagem " + std::to_string(network.side) + " × " +
			std::to_string(network.side) : "Entradas · " + std::to_string(network.inputs);
	}
	else
	{
		const Network::Layer& weights = network.layers[layer];
		bool last = layer + 1 == (int)network.layers.size();
		text = (last ? std::string("Saída") : "Oculta " + std::to_string(c)) + " · " +
			functionNames[weights.function] + " · " + std::to_string(weights.size);
	}
	// acima do bias dos neurônios e da seleção dos blocos; com o zoom afastado
	// o texto cresce, para continuar legível
	float size = ui::CaptionFontSize * std::min(MAX_CAPTION_SCALE, 1 / zoom);
	float height = size + 10;
	float above = (column.kind == Column::Nodes || column.kind == Column::Grid ?
		(c > 0 ? 71 : 35) : 15) + height / 2;
	Vector2 center = { (column.left + column.right) / 2, column.top - above };
	Rectangle box = draw::pillBox(center, text, size, height);
	bool columnSelected = isSelected(Part::Layer, layer, 0);
	if(columnSelected)
		draw::pill(box, ui::toColor(ui::AccentColor), ui::toColor(ui::AccentColor));
	else
		draw::pill(box, ui::toColor(ui::HoverColor), ui::toColor(ui::PanelBorderColor));
	draw::textCentered(text, center, size, columnSelected ? WHITE :
		ui::toColor(ui::StatusTextColor));
	boxes.push_back(Box{ box, Part{ Part::Layer, layer, 0, 0 } });
}

/**
 * Cor do peso no mapa: azul negativo, vermelho positivo, mais forte quanto
 * maior, cheia a partir de full
 */
static Color weightColor(float weight, float full)
{
	float t = full > 0 && std::isfinite(weight) ? fminf(fabsf(weight) / full, 1) : 0;
	return ColorLerp(WHITE, weight < 0 ? ui::toColor(ui::AccentColor) : Color{ 220, 38, 38, 255 },
		t);
}

/**
 * Peso que fica com a cor cheia: o dobro da raiz da média dos quadrados, para
 * uns poucos pesos grandes não apagarem os demais
 */
static float fullWeight(const float* weights, size_t count, size_t stride)
{
	double sum = 0;
	for(size_t i = 0; i < count; i++)
		sum += std::isfinite(weights[i * stride]) ? weights[i * stride] * weights[i * stride] : 0;
	return count > 0 ? (float)(2 * sqrt(sum / count)) : 0;
}

bool NeuralNetwork::isMosaic(int layer)
{
	return layer == 0 && network.side > 0;
}

void NeuralNetwork::updateTextures()
{
	if(heatmapsDirty)
	{
		for(Texture2D& texture : heatmaps)
		{
			if(texture.id != 0)
				UnloadTexture(texture);
		}
		heatmaps.assign(network.layers.size(), Texture2D{ 0, 0, 0, 0, 0 });
		for(size_t l = 0; l < network.layers.size(); l++)
		{
			if(columns[l + 1].kind != Column::Block)
				continue;
			const Network::Layer& layer = network.layers[l];
			int k = network.inputCount((int)l);
			Image map;
			if(isMosaic((int)l))
			{
				// os pesos de cada neurônio na forma da imagem, lado a lado
				int side = network.side;
				int across = (int)ceil(sqrt(layer.size));
				int down = (layer.size + across - 1) / across;
				map = GenImageColor(across * side, down * side, WHITE);
				Color* pixels = (Color*)map.data;
				for(int j = 0; j < layer.size; j++)
				{
					float full = fullWeight(&layer.weights[j], k, layer.size);
					int left = j % across * side, top = j / across * side;
					for(int i = 0; i < k; i++)
						pixels[(size_t)(top + i / side) * map.width + left + i % side] =
							weightColor(layer.weights[(size_t)i * layer.size + j], full);
				}
			}
			else
			{
				// uma linha por neurônio e uma coluna por entrada
				float full = fullWeight(layer.weights.data(), layer.weights.size(), 1);
				map = GenImageColor(k, layer.size, WHITE);
				Color* pixels = (Color*)map.data;
				for(int i = 0; i < k; i++)
				{
					for(int j = 0; j < layer.size; j++)
						pixels[(size_t)j * k + i] = weightColor(layer.weights[(size_t)i * layer.size + j],
							full);
				}
			}
			heatmaps[l] = LoadTextureFromImage(map);
			UnloadImage(map);
			// reduzido pela média dos pixels, sem serrilhar
			GenTextureMipmaps(&heatmaps[l]);
			SetTextureFilter(heatmaps[l], TEXTURE_FILTER_TRILINEAR);
		}
		heatmapsDirty = false;
	}
	bool frame = columns[0].kind == Column::Frame;
	if(image.id != 0 && (!frame || image.width != network.side))
	{
		UnloadTexture(image);
		image = Texture2D{ 0, 0, 0, 0, 0 };
	}
	if(!frame || (!imageDirty && image.id != 0))
		return;
	// pixels da imagem como as entradas: a cor acompanha a intensidade
	Image pixels = GenImageColor(network.side, network.side, WHITE);
	Color* colors = (Color*)pixels.data;
	for(int i = 0; i < network.inputs; i++)
		colors[i] = ColorLerp(WHITE, ui::toColor(ui::AccentDarkColor),
			fminf(fmaxf(inputs[i], 0), 1));
	if(image.id == 0)
	{
		image = LoadTextureFromImage(pixels);
		SetTextureFilter(image, TEXTURE_FILTER_POINT);
	}
	else
		UpdateTexture(image, pixels.data);
	UnloadImage(pixels);
	imageDirty = false;
}

void NeuralNetwork::unloadTextures()
{
	for(Texture2D& texture : heatmaps)
	{
		if(texture.id != 0)
			UnloadTexture(texture);
	}
	heatmaps.clear();
	if(image.id != 0)
		UnloadTexture(image);
	image = Texture2D{ 0, 0, 0, 0, 0 };
}

NeuralNetwork::Part NeuralNetwork::partAt(Vector2 point)
{
	// os desenhados por último ficam por cima
	for(auto it = boxes.rbegin(); it != boxes.rend(); it++)
	{
		if(CheckCollisionPointRec(point, it->box))
			return it->part;
	}
	for(size_t l = 0; l < network.layers.size() && l + 1 < columns.size(); l++)
	{
		const Column& from = columns[l];
		const Column& to = columns[l + 1];
		if(!to.lines)
			continue;
		for(int i = 0; i < from.count; i++)
		{
			for(int j = 0; j < to.count; j++)
			{
				// distância do ponto ao segmento
				Vector2 a = from.node(i), b = to.node(j);
				float dx = b.x - a.x, dy = b.y - a.y;
				float length = dx * dx + dy * dy;
				float t = length > 0 ? ((point.x - a.x) * dx + (point.y - a.y) * dy) / length : 0;
				t = t < 0 ? 0 : t > 1 ? 1 : t;
				if(hypotf(a.x + t * dx - point.x, a.y + t * dy - point.y) <= 4)
					return Part{ Part::Weight, (int)l, i, j };
			}
		}
	}
	return Part{ Part::None, 0, 0, 0 };
}

bool NeuralNetwork::isSelected(Part::Kind kind, int layer, int from, int to)
{
	if(selected.kind != kind || selected.layer != layer)
		return false;
	if(kind == Part::Layer)
		return true;
	if(kind == Part::Weight)
		return selected.from == from && selected.to == to;
	return selected.to == from;
}

void NeuralNetwork::select(Part part)
{
	selected = part;
}

void NeuralNetwork::execute()
{
	if(network.layers.empty())
		values.clear();
	else
		network.forward(inputs.data(), 1, values);
	imageDirty = true;
}

void NeuralNetwork::clearChanged()
{
	changedWeights.clear();
	changedBias.clear();
}

void NeuralNetwork::structureChanged(int oldInputs, int oldOutputs)
{
	inputs.resize(network.inputs, 0);
	targets.resize(network.outputs(), 0);
	// as amostras têm um valor por entrada e um alvo por saída
	if(network.inputs != oldInputs || network.outputs() != oldOutputs)
	{
		samples = Dataset(network.inputs, network.outputs());
		showingImage = false;
	}
	clearChanged();
	heatmapsDirty = true;
	imageDirty = true;
	modified = true;
	execute();
}

Vector2 NeuralNetwork::toWorld(int x, int y)
{
	return Vector2{ (x - pan.x) / zoom, (y - pan.y) / zoom };
}

Vector2 NeuralNetwork::toScreen(Vector2 point)
{
	return Vector2{ point.x * zoom + pan.x, point.y * zoom + pan.y };
}

Rectangle NeuralNetwork::toScreen(Rectangle rect)
{
	Vector2 corner = toScreen(Vector2{ rect.x, rect.y });
	return Rectangle{ corner.x, corner.y, rect.width * zoom, rect.height * zoom };
}

void NeuralNetwork::zoomAt(int x, int y, float wheel)
{
	float next = zoom * powf(ZOOM_STEP, wheel);
	next = next < MIN_ZOOM ? MIN_ZOOM : next > MAX_ZOOM ? MAX_ZOOM : next;
	pan.x = x - (x - pan.x) * next / zoom;
	pan.y = y - (y - pan.y) * next / zoom;
	zoom = next;
}

void NeuralNetwork::startPanning(int x, int y)
{
	panning = true;
	panX = x;
	panY = y;
}

void NeuralNetwork::startPan(int x, int y)
{
	startPanning(x, y);
}

void NeuralNetwork::endPan()
{
	panning = false;
}

bool NeuralNetwork::isMouseDown()
{
	return panning;
}

Rectangle NeuralNetwork::bounds()
{
	arrange();
	float left = columns.front().left, right = columns.back().right;
	float top = 0, bottom = 0;
	for(const Column& column : columns)
	{
		top = std::min(top, column.top);
		bottom = std::max(bottom, column.bottom);
	}
	if(columns.back().kind == Column::Nodes && !network.layers.empty())
		right += OUTPUT_GAP;
	return Rectangle{ left, top, right - left, bottom - top };
}

void NeuralNetwork::fitView(Rectangle area)
{
	Rectangle box = bounds();
	zoom = 1;
	if(box.width > area.width)
		zoom = area.width / box.width;
	if(box.height * zoom > area.height)
		zoom = area.height / box.height;
	zoom = std::max(zoom, MIN_ZOOM);
	// no pixel inteiro, para as linhas finas não borrarem
	pan.x = roundf(area.x + area.width / 2 - (box.x + box.width / 2) * zoom);
	pan.y = roundf(area.y + area.height / 2 - (box.y + box.height / 2) * zoom);
}

bool NeuralNetwork::isVisible(Rectangle area)
{
	Rectangle box = toScreen(bounds());
	// a rede de um nó só tem tamanho 0
	box = Rectangle{ box.x - 1, box.y - 1, box.width + 2, box.height + 2 };
	return CheckCollisionRecs(box, area);
}

float NeuralNetwork::getZoom()
{
	return zoom;
}

Vector2 NeuralNetwork::getPan()
{
	return pan;
}

void NeuralNetwork::setView(float zoom, Vector2 pan)
{
	this->zoom = zoom < MIN_ZOOM ? MIN_ZOOM : zoom > MAX_ZOOM ? MAX_ZOOM : zoom;
	this->pan = pan;
}

/**
 * Indica se as partes são a mesma
 */
static bool same(const NeuralNetwork::Part& a, const NeuralNetwork::Part& b)
{
	return a.kind == b.kind && a.layer == b.layer && a.from == b.from && a.to == b.to;
}

void NeuralNetwork::mouseDown(int x, int y)
{
	// o botão do meio já arrasta a vista
	if(panning)
		return;
	Part part = partAt(toWorld(x, y));
	if(running && part.kind == Part::Target)
	{
		targets[part.to] = targets[part.to] != 0 ? 0 : 1;
		clearChanged();
		execute();
		return;
	}
	if(running && part.kind == Part::Input)
	{
		inputs[part.to] = inputs[part.to] != 0 ? 0 : 1;
		showingImage = false;
		clearChanged();
		execute();
		return;
	}
	// o segundo clique no peso ou no bias selecionado o edita, e no nome da
	// camada muda a quantidade de neurônios
	bool editable = part.kind == Part::Weight || part.kind == Part::Bias ||
		(part.kind == Part::Layer && !running);
	if(editable && same(part, selected) && !isTraining())
	{
		for(const Box& box : boxes)
		{
			if(same(box.part, part))
				request = Request{ Request::Edit, part.layer, box.box };
		}
	}
	select(part);
	// no fundo vazio arrasta a vista
	if(part.kind == Part::None)
		startPanning(x, y);
}

void NeuralNetwork::mouseUp(int x, int y)
{
	panning = false;
}

void NeuralNetwork::mouseMove(int x, int y)
{
	if(!panning)
		return;
	pan.x += x - panX;
	pan.y += y - panY;
	panX = x;
	panY = y;
}

void NeuralNetwork::mouseDoubleClick(int x, int y)
{
	Part part = partAt(toWorld(x, y));
	// na execução cada clique na entrada ou no alvo alterna o valor
	if(isTraining() || (running && (part.kind == Part::Input || part.kind == Part::Target)))
		return;
	if(part.kind == Part::Weight || part.kind == Part::Bias)
	{
		select(part);
		for(const Box& box : boxes)
		{
			if(same(box.part, part))
				request = Request{ Request::Edit, part.layer, box.box };
		}
	}
	else if(part.kind != Part::None && part.kind != Part::Layer)
		mouseRightDown(x, y);
}

void NeuralNetwork::mouseRightDown(int x, int y)
{
	Vector2 point = toWorld(x, y);
	Part part = partAt(point);
	// o menu do bias é o do neurônio
	if(part.kind == Part::Bias)
		part.kind = Part::Neuron;
	select(part);
	if(!isTraining() && (part.kind == Part::Input || part.kind == Part::Neuron ||
		part.kind == Part::Layer))
		request = Request{ Request::Menu, part.layer, Rectangle{ point.x, point.y, 0, 0 } };
}

void NeuralNetwork::keyDown(int key)
{
	if(key == KEY_ESCAPE)
		select(Part{ Part::None, 0, 0, 0 });
	// no teclado do Mac a tecla delete é o backspace
	else if((key == KEY_DELETE || key == KEY_BACKSPACE) && !running && !isTraining())
		removeSelected();
}

void NeuralNetwork::addInput()
{
	int oldInputs = network.inputs, oldOutputs = network.outputs();
	network.setInputs(network.inputs + 1, 0, random);
	structureChanged(oldInputs, oldOutputs);
	select(Part{ Part::Input, -1, network.inputs - 1, network.inputs - 1 });
}

/**
 * Camada da parte selecionada, -1 sem uma
 */
static int layerOf(const NeuralNetwork::Part& part)
{
	return part.kind == NeuralNetwork::Part::None || part.kind == NeuralNetwork::Part::Input ?
		-1 : part.layer;
}

void NeuralNetwork::addNeuron()
{
	int oldInputs = network.inputs, oldOutputs = network.outputs();
	int count = (int)network.layers.size();
	int layer = layerOf(selected) >= 0 ? selected.layer : count - 2;
	if(count == 0)
	{
		network.insertLayer(0, 1, Network::Linear, random);
		layer = 0;
	}
	else if(layer < 0)
	{
		// sem oculta, cria uma antes da saída
		layer = count - 1;
		network.insertLayer(layer, 1, Network::Logistic, random);
	}
	else
		network.resize(layer, network.layers[layer].size + 1, random);
	structureChanged(oldInputs, oldOutputs);
	int last = network.layers[layer].size - 1;
	select(Part{ Part::Neuron, layer, last, last });
}

void NeuralNetwork::addLayer()
{
	int oldInputs = network.inputs, oldOutputs = network.outputs();
	int count = (int)network.layers.size();
	int index = 0;
	if(count == 0)
		network.insertLayer(0, 1, Network::Linear, random);
	else
	{
		// depois da oculta selecionada ou antes da saída
		int layer = layerOf(selected);
		index = layer >= 0 && layer < count - 1 ? layer + 1 : count - 1;
		network.insertLayer(index, 2, Network::Logistic, random);
	}
	structureChanged(oldInputs, oldOutputs);
	select(Part{ Part::Layer, index, 0, 0 });
}

void NeuralNetwork::addOutput()
{
	int oldInputs = network.inputs, oldOutputs = network.outputs();
	if(network.layers.empty())
		network.insertLayer(0, 1, Network::Linear, random);
	else
		network.resize((int)network.layers.size() - 1, network.outputs() + 1, random);
	structureChanged(oldInputs, oldOutputs);
	int layer = (int)network.layers.size() - 1, last = network.outputs() - 1;
	select(Part{ Part::Neuron, layer, last, last });
}

void NeuralNetwork::removeSelected()
{
	int oldInputs = network.inputs, oldOutputs = network.outputs();
	if(selected.kind == Part::Input)
		network.removeInput(selected.to);
	else if(selected.kind == Part::Neuron || selected.kind == Part::Bias)
		network.removeNeuron(selected.layer, selected.to, random);
	else if(selected.kind == Part::Layer && selected.layer >= 0)
		network.removeLayer(selected.layer, random);
	else
		return;
	select(Part{ Part::None, 0, 0, 0 });
	structureChanged(oldInputs, oldOutputs);
}

void NeuralNetwork::removeLayer(int layer)
{
	select(Part{ Part::Layer, layer, 0, 0 });
	removeSelected();
}

void NeuralNetwork::setFunction(int layer, int function)
{
	network.layers[layer].function = function;
	clearChanged();
	modified = true;
	execute();
}

void NeuralNetwork::setImageSide(int side)
{
	int oldInputs = network.inputs, oldOutputs = network.outputs();
	if(side > 0)
		network.setInputs(side * side, side, random);
	else
		network.side = 0;
	structureChanged(oldInputs, oldOutputs);
	select(Part{ Part::Layer, -1, 0, 0 });
}

float NeuralNetwork::getSelectedValue()
{
	if(selected.kind == Part::Layer && selected.layer < 0)
		return network.side > 0 ? network.side : network.inputs;
	const Network::Layer& layer = network.layers[selected.layer];
	if(selected.kind == Part::Layer)
		return layer.size;
	if(selected.kind == Part::Weight)
		return layer.weights[(size_t)selected.from * layer.size + selected.to];
	return layer.bias[selected.to];
}

bool NeuralNetwork::setSelectedValue(float value)
{
	if((selected.kind != Part::Weight && selected.kind != Part::Bias &&
		selected.kind != Part::Layer) || value == getSelectedValue())
		return false;
	if(selected.kind == Part::Layer)
	{
		// a quantidade de neurônios, de entradas ou o lado da imagem
		int count = (int)lroundf(value), oldInputs = network.inputs, oldOutputs = network.outputs();
		Part part = selected;
		if(selected.layer >= 0 && count >= 1 && count <= MAX_LAYER_SIZE)
			network.resize(selected.layer, count, random);
		else if(selected.layer < 0 && network.side > 0 && count >= 2 && count <= MAX_IMAGE_SIDE)
			network.setInputs(count * count, count, random);
		else if(selected.layer < 0 && network.side == 0 && count >= 0 && count <= MAX_LAYER_SIZE)
			network.setInputs(count, 0, random);
		else
			return false;
		structureChanged(oldInputs, oldOutputs);
		select(part);
		return true;
	}
	Network::Layer& layer = network.layers[selected.layer];
	if(selected.kind == Part::Weight)
		layer.weights[(size_t)selected.from * layer.size + selected.to] = value;
	else
		layer.bias[selected.to] = value;
	clearChanged();
	heatmapsDirty = true;
	modified = true;
	execute();
	return true;
}

void NeuralNetwork::saveToFile(const std::string& fileName)
{
	std::filesystem::path path = pathFromUtf8(fileName);
	network.save(path);
	samples.saveSamples(path);
	modified = false;
}

int NeuralNetwork::loadSamples(const std::string& fileName)
{
	samples = Dataset(network.inputs, network.outputs());
	samples = Dataset::loadSamples(pathFromUtf8(fileName), network.inputs, network.outputs());
	return samples.size();
}

void NeuralNetwork::loadFromFile(const std::string& fileName)
{
	// lê o arquivo inteiro antes de alterar a rede, um arquivo inválido
	// mantém a rede atual
	Network loaded = Network::load(pathFromUtf8(fileName));
	cancelTraining();
	network = loaded;
	samples = Dataset(network.inputs, network.outputs());
	inputs.assign(network.inputs, 0);
	targets.assign(network.outputs(), 0);
	select(Part{ Part::None, 0, 0, 0 });
	showingImage = false;
	clearChanged();
	heatmapsDirty = true;
	imageDirty = true;
	modified = false;
	panning = false;
	request = Request{ Request::None, 0, Rectangle{ 0, 0, 0, 0 } };
	execute();
}

int NeuralNetwork::loadData(const std::string& path)
{
	std::filesystem::path file = pathFromUtf8(path);
	if(network.side == 0)
		throw std::runtime_error("As entradas da rede precisam ser uma imagem para treinar com "
			"imagens: clique direito nas entradas.");
	// com a redução das imagens arrastadas, em qualquer lado
	Dataset loaded, loadedTest;
	if(Dataset::isIdx(file))
	{
		loaded = pixels::fromIdx(file, network.side);
		std::filesystem::path testPath = Dataset::testPath(file);
		if(!testPath.empty())
			loadedTest = pixels::fromIdx(testPath, network.side);
	}
	else
		loaded = pixels::fromFolder(path, network.side, network.outputs());
	cancelTraining();
	data = std::move(loaded);
	test = std::move(loadedTest);
	dataName = pathToUtf8(file.filename());
	return data.size();
}

const std::string& NeuralNetwork::getDataName()
{
	return dataName;
}

int NeuralNetwork::getDataCount()
{
	return data.size();
}

int NeuralNetwork::getTestCount()
{
	return test.size();
}

bool NeuralNetwork::isModified()
{
	return modified;
}

NeuralNetwork::Part NeuralNetwork::getSelected()
{
	return selected;
}

const Network& NeuralNetwork::getNetwork()
{
	return network;
}

NeuralNetwork::Request NeuralNetwork::takeRequest()
{
	Request result = request;
	request = Request{ Request::None, 0, Rectangle{ 0, 0, 0, 0 } };
	return result;
}

void NeuralNetwork::setRunning(bool running)
{
	this->running = running;
	showingImage = showingImage && running;
	panning = false;
	clearChanged();
	execute();
}

bool NeuralNetwork::isRunning()
{
	return running;
}

void NeuralNetwork::train(bool withScreen)
{
	if(job)
		return;
	clearChanged();
	job.reset(new Job());
	Job* current = job.get();
	current->network = network;
	if(!data.empty())
	{
		current->data = &data;
		current->test = test.empty() ? nullptr : &test;
	}
	else
	{
		// guarda a combinação da tela; com softmax, cujas saídas somam 1, só se
		// algum alvo for diferente de 0
		bool hasTarget = std::any_of(targets.begin(), targets.end(),
			[](float target) { return target != 0; });
		if(withScreen && !network.layers.empty() && (hasTarget ||
			network.layers.back().function != Network::Softmax) && samples.remember(inputs, targets))
			modified = true;
		current->samples = samples;
		current->data = &current->samples;
	}
	current->trainer.progress = [current](const Trainer::Progress& progress)
	{
		std::lock_guard<std::mutex> lock(current->mutex);
		current->progress = progress;
		current->hasProgress = true;
	};
	current->thread = std::thread([current]()
	{
		current->result = current->trainer.train(current->network, *current->data, current->test);
		current->finished = true;
	});
}

bool NeuralNetwork::isTraining()
{
	return job != nullptr;
}

void NeuralNetwork::stopTraining()
{
	if(job)
		job->trainer.stop = true;
}

void NeuralNetwork::cancelTraining()
{
	if(!job)
		return;
	job->trainer.stop = true;
	job->thread.join();
	job.reset();
}

bool NeuralNetwork::getProgress(Trainer::Progress& progress)
{
	if(!job)
		return false;
	std::lock_guard<std::mutex> lock(job->mutex);
	progress = job->progress;
	return job->hasProgress;
}

bool NeuralNetwork::takeResult(Trainer::Result& result)
{
	if(!job || !job->finished)
		return false;
	job->thread.join();
	result = job->result;
	if(result.changed)
	{
		// marca o que o treino mudou
		const Network& trained = job->network;
		changedWeights.assign(network.layers.size(), std::vector<char>());
		changedBias.assign(network.layers.size(), std::vector<char>());
		for(size_t l = 0; l < network.layers.size(); l++)
		{
			for(size_t i = 0; i < trained.layers[l].weights.size(); i++)
				changedWeights[l].push_back(trained.layers[l].weights[i] !=
					network.layers[l].weights[i]);
			for(size_t j = 0; j < trained.layers[l].bias.size(); j++)
				changedBias[l].push_back(trained.layers[l].bias[j] != network.layers[l].bias[j]);
		}
		network = trained;
		heatmapsDirty = true;
		modified = true;
	}
	job.reset();
	execute();
	return true;
}

void NeuralNetwork::randomizeWeights()
{
	network.randomize(random);
	clearChanged();
	heatmapsDirty = true;
	modified = true;
	execute();
}

int NeuralNetwork::getSampleCount()
{
	return samples.size();
}

void NeuralNetwork::forgetSamples()
{
	cancelTraining();
	if(!samples.empty())
		modified = true;
	samples = Dataset(network.inputs, network.outputs());
	data = Dataset();
	test = Dataset();
	dataName.clear();
}

bool NeuralNetwork::isEmpty()
{
	return network.inputs == 0 && network.layers.empty();
}

double NeuralNetwork::getError()
{
	double error = 0;
	for(size_t o = 0; !values.empty() && o < targets.size(); o++)
		error += fabsf(targets[o] - values.back()[o]);
	return error;
}

bool NeuralNetwork::isOnTarget()
{
	for(size_t o = 0; !values.empty() && o < targets.size(); o++)
	{
		if(!(fabsf(targets[o] - values.back()[o]) < Network::Tolerance))
			return false;
	}
	return true;
}

int NeuralNetwork::getImageSide()
{
	return network.side;
}

bool NeuralNetwork::showImage(const std::vector<float>& values, int label)
{
	running = true;
	showingImage = true;
	panning = false;
	clearChanged();
	std::copy(values.begin(), values.begin() + std::min(values.size(), inputs.size()),
		inputs.begin());
	for(size_t o = 0; o < targets.size(); o++)
		targets[o] = (int)o == label ? 1 : 0;
	execute();
	return label >= 0 && label < (int)targets.size();
}

bool NeuralNetwork::isShowingImage()
{
	return showingImage;
}

int NeuralNetwork::getRecognized()
{
	if(values.empty())
		return -1;
	return Trainer::recognized(values.back().data(), (int)values.back().size());
}

int NeuralNetwork::getExpected()
{
	int expected = -1;
	for(size_t o = 0; o < targets.size(); o++)
	{
		if(targets[o] == 0)
			continue;
		if(expected >= 0)
			return -1;
		expected = (int)o;
	}
	return expected;
}
