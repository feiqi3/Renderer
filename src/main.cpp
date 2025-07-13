#include "vulkan/vulkan_render_function.h"
#include "render_resource_createInfo.h"
#include "window/render_resource_window_glfw.h"
int main() {
	using namespace Render::Vulkan;
	using namespace Render;
	BackEndInitDesc backEndDesc;
	Window::rs_window_glfw* window = new Window::rs_window_glfw("Hello world", 1920, 1080);
	backEndDesc.appName = "Test";
	backEndDesc.engineName = "Test";
	initVulkanBackEnd(backEndDesc, window);

}