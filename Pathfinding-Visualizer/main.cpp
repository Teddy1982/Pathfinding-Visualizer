#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "src/vulkan/renderData.h"
#include "src/application/application.h"


int main() {
	App app;
	app.init();
	app.loop();
	app.cleanup();
}
