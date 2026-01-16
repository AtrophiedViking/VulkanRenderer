#include "headers/application.h"

int main() {
	State state{
		.config{
			.windowTitle = "Vulkan Triangle",
			.windowResizable = true,
			.windowWidth = 800,
			.windowHeight = 600,
			.swapchainBuffering = SWAPCHAIN_TRIPPLE_BUFFERING,
			.backgroundColor = {0.04f,0.015f,0.04f},
			.msaaSamples = VK_SAMPLE_COUNT_1_BIT,
			.TEXTURE_PATH = "res/textures/viking_room.png",
			.MODEL_PATH = "res/models/viking_room.obj",
		}
	};
	init(&state);
	mainloop(&state);
	cleanup(&state);
};