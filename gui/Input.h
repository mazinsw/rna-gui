#ifndef _INPUT_H_
#define _INPUT_H_

/**
 * Estado do botão esquerdo do mouse por quadro
 *
 * O raylib só guarda o último estado do botão, então um toque no trackpad
 * (pressiona e solta no mesmo quadro) se perde. Aqui os eventos do GLFW são
 * acumulados entre os quadros para que nenhum clique seja descartado.
 */
namespace input
{
	/**
	 * Intercepta os eventos de botão do mouse, chamar após InitWindow
	 */
	void install();

	/**
	 * Consolida os eventos recebidos desde o quadro anterior
	 */
	void beginFrame();

	/**
	 * O botão foi pressionado desde o quadro anterior
	 */
	bool mousePressed();

	/**
	 * O botão foi solto desde o quadro anterior
	 */
	bool mouseReleased();

	/**
	 * O botão está pressionado agora
	 */
	bool mouseDown();

	/**
	 * O segundo clique de um clique duplo ocorreu desde o quadro anterior
	 */
	bool mouseDoubleClicked();
}

#endif /* _INPUT_H_ */
