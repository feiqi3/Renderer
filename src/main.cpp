#include "vulkan/vulkan_render_function.h"
#include "render_resource_createInfo.h"
#include "window/render_resource_window_glfw.h"

void renderLoop(Render::Vulkan::rs_context_vk* render_context,Render::Window::rs_window_glfw* window);
void initSomeThing();

int main() {
	using namespace Render::Vulkan;
	using namespace Render;
	BackEndInitDesc backEndDesc;
	Window::rs_window_glfw* window = new Window::rs_window_glfw("Hello world", 1920, 1080);
	backEndDesc.appName = "Test";
	backEndDesc.engineName = "Test";
	backEndDesc.enableValidation = true;
	auto ctx = initVulkanBackEnd(backEndDesc, window);
	deinitVulkanBackEnd(ctx, window);
}

void renderLoop(Render::Vulkan::rs_context_vk* render_context, Render::Window::rs_window_glfw* window)
{
	using namespace Render::Vulkan;
	using namespace Render;
	while (!window->shouldClose()) {
		window->pollEvents();



	};
}

void initSomeThing()
{
	using namespace Render::Vulkan;
	using namespace Render;
}
