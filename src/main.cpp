
#include "logic.h"

int main(int argc, char** argv) {

	//profiler = new GpuProfiler(true);

	setupGraphics(0, 0);

	int counter = 0;

	while (!glfwWindowShouldClose(window)) {

		glfwPollEvents();

		HandleInput();
		
		renderFrame();

		glfwSwapBuffers(window);


	}

	glfwTerminate();
	exit(EXIT_SUCCESS);
}