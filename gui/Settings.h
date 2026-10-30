#ifndef _SETTINGS_H_
#define _SETTINGS_H_
#include <raylib.h>
#include <string>

// menor tamanho da janela, o painel de botões e o status precisam caber na
// área de edição
#define WINDOW_MIN_WIDTH 560
#define WINDOW_MIN_HEIGHT 440

/**
 * Estado guardado entre as execuções: a janela e a vista da última rede
 * aberta
 */
struct Settings
{
	// posição e tamanho da janela fora de maximizada, sem a posição o raylib
	// centraliza a janela
	bool hasPosition;
	int x, y, width, height;
	bool maximized;
	// arquivo da rede com o zoom e o deslocamento abaixo, vazio sem vista
	std::string fileName;
	float zoom;
	Vector2 pan;
};

/**
 * Lê o estado gravado, um arquivo ausente ou inválido dá o estado inicial
 */
Settings loadSettings();

/**
 * Grava o estado, uma falha só perde o estado
 */
void saveSettings(const Settings& settings);

/**
 * Leva a janela à posição gravada, se ainda estiver em algum monitor, e a
 * maximiza. Chamar após InitWindow com o tamanho gravado.
 */
void restoreWindow(const Settings& settings);

/**
 * Atualiza o estado com a janela. Maximizada ou minimizada, fica a posição e
 * o tamanho de antes, para onde ela volta.
 */
void captureWindow(Settings& settings);

#endif /* _SETTINGS_H_ */
