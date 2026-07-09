#include"Renderer/RenderSystem.h"
#include "window/render_resource_window_glfw.h"
#include "Renderer/RenderFlow.h"
#include "Renderer/ObjectEntity.h"
#include <chrono>
#include <iostream>
#include <string>
#include "common/StringPool.h"
#include "Renderer/MaterialTemplateManager.h"
#include "platform/FileSystem/WinFileSystem.h"
#include "Renderer/CameraManager.h"
#include "Renderer/Camera.h"
#include "common/ResourceSystem.h"
#include "function/EngineResourceManager.h"
#include "Renderer/NaiveScene.h"
#include "function/InputManager.h"
#include "Renderer/DebugDrawManager.h"
void setWindowEventsCallbacks(Render::Window::rs_window_glfw* window);

int main() {
	using namespace Render;
	Render::BackEndInitDesc backEndInit{};
	backEndInit.appName = "Test";
	backEndInit.engineName = "Feigen";
	new Render::ResourceSystem();
	Render::Window::rs_window_glfw* glfwWindow = new Render::Window::rs_window_glfw("Hello world", 800, 600);
	new InputManager;
	InputManager::instance()->initByWindowSystem(glfwWindow);

	Render::RenderSystem::createRenderSystem(backEndInit, glfwWindow);

	setWindowEventsCallbacks(glfwWindow);
	Render::RenderFlow* renderFlow = new Render::RenderFlow();
	auto renderSystem = Render::RenderSystem::instance();
	CreateAllPersistentResource();
	new DebugDrawManager;
	renderFlow->init();
	float FrameTime = 0.f;
	int CurrentFPS = 0;
	float TargetFps = 60.;
	float TargetFrameTime = 1000. / TargetFps;
	auto* naiveScene = createSceneByCode();

	while (!glfwWindow->shouldClose()) {
		auto frameBegin = std::chrono::steady_clock::now();
		Render::RenderSystem::instance()->beginFrame();

		//scene->updateScene(0.01666);
		InputManager::instance()->preUpdate();
		glfwWindow->pollEvents();
		InputManager::instance()->postUpdate();
		naiveScene->update(0.166666);
		renderFlow->Excute();
		renderSystem->EndLogicFrame();
		renderSystem->BeginRenderFrame();
		auto frameEnd = std::chrono::steady_clock::now();
		auto targetDuration = std::chrono::microseconds(static_cast<int64_t>(TargetFrameTime * 1000.f));
		auto targetEndFrameTime = frameBegin + targetDuration;
		std::this_thread::sleep_until(targetEndFrameTime);

		auto frameNewEnd = std::chrono::steady_clock::now();
		auto frameNewDuration = frameNewEnd - frameBegin;
		float frameNewTime = (std::chrono::duration_cast<std::chrono::microseconds>(frameNewDuration).count()) / 1000.f;
		CurrentFPS = int(1000.f / frameNewTime);
		glfwWindow->setTitle((std::string("fps: ") + std::to_string(CurrentFPS)).c_str());
	}
	delete naiveScene;
	delete DebugDrawManager::instance();
	renderFlow->deinit();
	Render::RenderSystem::destroyRenderSystem();
	InputManager::instance()->deinitByWindowSystem(glfwWindow);
	delete InputManager::instance();
	delete glfwWindow;
	glfwWindow = 0;
}

void setWindowEventsCallbacks(Render::Window::rs_window_glfw* window)
{
	//1. OnResize
	window->ResizeEvent += [](int x, int y) {
		std::cout << "Window Resize: x:" << x << " y:" << y << "\n";
		Render::RenderSystem::instance()->onWindowResize(x,y);
		return true;
		};
}
