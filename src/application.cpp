#include "headers/application.h"

void init(State *state) {
	errorHandlingSetup(state);
	logPrint(state);
	windowCreate(state);
};

void mainloop(State *state) {
	while (!glfwWindowShouldClose(state->window.handle)) {
		glfwPollEvents();
		processInput(state);

		gameObjectsSort(state);
		uniformBuffersUpdate(state);

		frameDraw(state);
	};
		vkDeviceWaitIdle(state->context.device);
};

void cleanup(State *state) {
	windowDestroy(state);
};