#include"Renderer/RenderSystem.h"
#include "window/render_resource_window_glfw.h"
#include "Renderer/RenderFlow.h"
#include "Renderer/ObjectEntity.h"
#include <chrono>
#include <iostream>

void setWindowEventsCallbacks(Render::Window::rs_window_glfw* window);

int main() {
	Render::BackEndInitDesc backEndInit{};
	backEndInit.appName = "Test";
	backEndInit.engineName = "Feigen";

	Render::Window::rs_window_glfw* glfwWindow = new Render::Window::rs_window_glfw("Hello world", 800, 600);
	Render::RenderSystem::createRenderSystem(backEndInit, glfwWindow);
	setWindowEventsCallbacks(glfwWindow);
	Render::RenderFlow* renderFlow = new Render::RenderFlow();
	auto renderSystem = Render::RenderSystem::instance();
	renderFlow->init();

	float FrameTime = 0.f;
	int CurrentFPS = 0;
	int TargetFps = 60;
	float TargetFrameTime = 1000. / TargetFps;

	while (!glfwWindow->shouldClose()) {
		auto frameBegin = std::chrono::system_clock::now();
		Render::RenderSystem::instance()->beginFrame();

		if (Render::RenderSystem::instance()->getNextRenderFrame() == 0) {
			auto Cube = new Render::CubeEntity();
			renderFlow->AddEntity(Cube);
		}

		renderFlow->Excute();
		renderSystem->EndLogicFrame();
		glfwWindow->pollEvents();
		renderSystem->BeginRenderFrame();
		auto frameEnd = std::chrono::system_clock::now();
		auto frameDuration = frameEnd - frameBegin;
		FrameTime = (std::chrono::duration_cast<std::chrono::microseconds>(frameDuration).count()) / 1000.f;
		float sleepTime = TargetFrameTime - FrameTime;
		if (sleepTime > 0.5) {
			std::chrono::duration<float, std::milli> ms_dur(sleepTime);
			std::this_thread::sleep_for(ms_dur);
		}
		auto frameNewEnd = std::chrono::system_clock::now();
		auto frameNewDuration = frameNewEnd - frameBegin;
		float frameNewTime = (std::chrono::duration_cast<std::chrono::microseconds>(frameNewDuration).count()) / 1000.f;
		CurrentFPS = int(1000. / frameNewTime);
		std::cout << "Current FPS:" << CurrentFPS << "\n";
	}
	renderFlow->deinit();
	Render::RenderSystem::destroyRenderSystem();
}

void setWindowEventsCallbacks(Render::Window::rs_window_glfw* window)
{
	//1. OnResize
	window->ResizeEvent += [](int x, int y) {
		std::cout << "Window Resize: x:" << x << " y:" << y << "\n";
		Render::RenderSystem::instance()->onWindowResize();
		return true;
		};
}
