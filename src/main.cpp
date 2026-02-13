#include "headers/application.h"

int main() {
	State state{
		.config{
			.windowTitle = "Vulkan Triangle",
			.windowResizable = true,
			.windowWidth = 800,
			.windowHeight = 600,
			.swapchainBuffering = SWAPCHAIN_TRIPPLE_BUFFERING,
			.MAX_OBJECTS = 3,
			.backgroundColor = {0.04f,0.015f,0.04f},
			.msaaSamples = VK_SAMPLE_COUNT_1_BIT,
			.KOBOLD_TEXTURE_PATH = "res/textures/skin.ktx2",
			.KOBOLD_MODEL_PATH = "res/models/hover_bike.glb",
		}
	};
	init(&state);
	mainloop(&state);
	cleanup(&state);
};