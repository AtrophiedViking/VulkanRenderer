#include "headers/application.h"

int main() {
	State state{
		.config{
			.windowTitle = "Vulkan Triangle",
			.windowResizable = false,
			.windowWidth = 800,
			.windowHeight = 600, 
			.swapchainBuffering = SWAPCHAIN_TRIPPLE_BUFFERING,
		}
	};
	init(&state);
	mainloop(&state);
	cleanup(&state);
};