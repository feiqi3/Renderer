#include"Renderer/RenderSystem.h"
#include "window/render_resource_window_glfw.h"
#include "Renderer/RenderFlow.h"
#include "Renderer/ObjectEntity.h"

int main() {
	Render::BackEndInitDesc backEndInit{};
	backEndInit.appName = "Test";
	backEndInit.engineName = "Feigen";

	Render::Window::rs_window_glfw* glfwWindow = new Render::Window::rs_window_glfw("Hello world", 800, 600);

	Render::RenderSystem::createRenderSystem(backEndInit, glfwWindow);

	Render::RenderFlow* renderFlow = new Render::RenderFlow();
	auto renderSystem = Render::RenderSystem::instance();
	renderSystem->initSwapchainRT();
	renderFlow->init();



	while (!glfwWindow->shouldClose()) {
		Render::RenderSystem::instance()->beginFrame();

		if (Render::RenderSystem::instance()->getNextRenderFrame() == 1) {
			auto Cube = new Render::CubeEntity();
			renderFlow->AddEntity(Cube);
		}

		renderFlow->Excute();
		renderSystem->EndLogicFrame();
		renderSystem->BeginRenderFrame();
	}

	renderFlow->deinit();

	Render::RenderSystem::destroyRenderSystem();
}

