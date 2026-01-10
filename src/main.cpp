#include "headers/application.h"

int main() {
	State state{
		.config{
			.windowTitle = "Vulkan Triangle",
			.windowResizable = true,
			.windowWidth = 800,
			.windowHeight = 600, 
			.swapchainBuffering = SWAPCHAIN_TRIPPLE_BUFFERING,
		}
	};
	init(&state);
	mainloop(&state);
	cleanup(&state);
};