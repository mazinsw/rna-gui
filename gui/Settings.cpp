#include "Settings.h"
#include "Platform.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

/*
 * Formato do arquivo, texto com um valor por linha, as linhas desconhecidas ou
 * inválidas são ignoradas:
 * posicao x y
 * tamanho largura altura
 * maximizada 0|1
 * rede caminho em UTF-8
 * vista zoom deslocamentoX deslocamentoY
 */

// a janela volta para a posição gravada se pelo menos esta faixa do topo
// estiver num monitor, para ainda poder ser arrastada
#define VISIBLE_WIDTH 100
#define VISIBLE_HEIGHT 40

Settings loadSettings()
{
	Settings settings = { false, 0, 0, 800, 600, false, "", 1, { 0, 0 } };
	std::ifstream file(pathFromUtf8(getSettingsFilePath()), std::ios::binary);
	std::string line, fileName;
	bool hasView = false;
	while(std::getline(file, line))
	{
		std::istringstream values(line);
		std::string key;
		int a, b;
		float zoom;
		Vector2 pan;
		values >> key;
		if(key == "posicao" && values >> a >> b)
		{
			settings.hasPosition = true;
			settings.x = a;
			settings.y = b;
		}
		else if(key == "tamanho" && values >> a >> b)
		{
			settings.width = std::max(a, WINDOW_MIN_WIDTH);
			settings.height = std::max(b, WINDOW_MIN_HEIGHT);
		}
		else if(key == "maximizada" && values >> a)
			settings.maximized = a != 0;
		else if(key == "rede")
			std::getline(values >> std::ws, fileName);
		else if(key == "vista" && values >> zoom >> pan.x >> pan.y && zoom > 0 &&
			std::isfinite(zoom) && std::isfinite(pan.x) && std::isfinite(pan.y))
		{
			hasView = true;
			settings.zoom = zoom;
			settings.pan = pan;
		}
	}
	// a vista sem a rede dela não serve para nenhuma
	if(hasView)
		settings.fileName = fileName;
	return settings;
}

void saveSettings(const Settings& settings)
{
	std::ofstream file(pathFromUtf8(getSettingsFilePath()), std::ios::binary | std::ios::trunc);
	// dígitos para o float ser lido igual
	file.precision(9);
	if(settings.hasPosition)
		file << "posicao " << settings.x << " " << settings.y << "\n";
	file << "tamanho " << settings.width << " " << settings.height << "\n";
	file << "maximizada " << (settings.maximized ? 1 : 0) << "\n";
	if(!settings.fileName.empty())
	{
		file << "rede " << settings.fileName << "\n";
		file << "vista " << settings.zoom << " " << settings.pan.x << " " << settings.pan.y << "\n";
	}
}

/**
 * Indica se o topo da janela na posição fica em algum monitor
 */
static bool isOnScreen(int x, int y, int width)
{
	for(int i = 0; i < GetMonitorCount(); i++)
	{
		Vector2 corner = GetMonitorPosition(i);
		int left = std::max(x, (int)corner.x);
		int right = std::min(x + width, (int)corner.x + GetMonitorWidth(i));
		if(right - left >= VISIBLE_WIDTH && y >= corner.y &&
			y + VISIBLE_HEIGHT <= corner.y + GetMonitorHeight(i))
			return true;
	}
	return false;
}

void restoreWindow(const Settings& settings)
{
	int monitor = GetCurrentMonitor();
	int monitorWidth = GetMonitorWidth(monitor);
	int monitorHeight = GetMonitorHeight(monitor);
	if(settings.hasPosition && isOnScreen(settings.x, settings.y, settings.width))
		SetWindowPosition(settings.x, settings.y);
	else if(monitorWidth > 0 && monitorHeight > 0 &&
		(settings.width > monitorWidth || settings.height > monitorHeight))
	{
		// gravada num monitor maior que não está mais ligado: centralizada no
		// atual, com uma margem
		int width = std::max(std::min(settings.width, monitorWidth * 9 / 10), WINDOW_MIN_WIDTH);
		int height = std::max(std::min(settings.height, monitorHeight * 9 / 10),
			WINDOW_MIN_HEIGHT);
		Vector2 corner = GetMonitorPosition(monitor);
		SetWindowSize(width, height);
		SetWindowPosition((int)corner.x + (monitorWidth - width) / 2,
			(int)corner.y + (monitorHeight - height) / 2);
	}
	if(settings.maximized)
		MaximizeWindow();
}

void captureWindow(Settings& settings)
{
	if(IsWindowMinimized())
		return;
	settings.maximized = IsWindowMaximized();
	if(settings.maximized)
		return;
	Vector2 position = GetWindowPosition();
	settings.hasPosition = true;
	settings.x = (int)position.x;
	settings.y = (int)position.y;
	settings.width = GetScreenWidth();
	settings.height = GetScreenHeight();
}
