#include "Window.h"
#include <fstream>

#include <glad/glad.h>
#include <functional>

static void initGlfw() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

Window::Window(size_t width, size_t height, const char* title, Renderer& renderer) :
	renderer(renderer), mWidth(width), mHeight(height) {// preguntar por el uso de :

	initGlfw();
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	glfwMakeContextCurrent(window);
	gladLoadGL();
	glViewport(0, 0, width, height);
	renderer.setWindow(this);// 
	// se inicializa todo con sus handlers despues
	static Renderer& r = renderer;// agrega renderer a una variable estatica
	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		r.handleKey(
			key == 340 ? 'Z' : key, // remap left shift to Z
			action
		);
		});
	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
		r.handleMouseMove(xpos, ypos);
		});
	glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		r.handleMouse(button, action, x, y);
		});
}

Window::~Window() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Window::swapBuffers() const noexcept {
	glfwSwapBuffers(window);
}

void Window::mainLoop() {
	//std::fstream fps("fps.txt", std::fstream::out);
	double lastTime = glfwGetTime();
	//double ini = glfwGetTime();
	//double intervalo = 120.0;
	int nFrames = 0;
	renderer.init();
	int position = 0;
	glfwSwapInterval(0);
	bool stopflag =false;
	while (!glfwWindowShouldClose(window)) { // main loop
		
		glfwPollEvents();
		
		renderer.update(); //eventos teclado
		//renderer.automaticCam(position,stopflag);// leer text.txt movimientos camara
		//if (stopflag) break;// si termina el vector
		//position++;
		
		renderer.render();
		swapBuffers();
		
		double currentTime = glfwGetTime();
		double now = glfwGetTime();
		
		
		//if (now - ini >= intervalo) { // grabar movimientos de ca
		//	renderer.changeFlag(false);
		//	break;
		//}
		nFrames++;
		if (currentTime - lastTime >= 1.0) {
			//printf("%d fps\n", int(nFrames));
			//fps << nFrames << '\n';
			nFrames = 0;
			lastTime = currentTime;
		}
	}
}