#include "function/InputManager.h"
#include <cstring>
#include <iostream>
#include "Renderer/RenderSystem.h" 
#include "window/render_resource_window_glfw.h"
namespace Render {
	bool isKeyStatePressed(uint16_t keyState) {
		return (keyState & 0x00FF) != 0;
	}

	uint16_t packKeyState(KeyModifierFlags flag, bool pressed) {
		return (static_cast<uint16_t>(flag) << 8) | static_cast<uint16_t>(pressed);
	}

	void unpackKeyState(uint16_t keyState, KeyModifierFlags& flag, bool& pressed) {
		flag = static_cast<KeyModifierFlags>(keyState >> 8);
		pressed = (keyState & 0xFF) != 0;
	}

	Render::InputManager::InputManager()
	{
		mPrevKeyState = new u16[MAX_KEY_CODE];
		mCurrKeyState = new u16[MAX_KEY_CODE];

		int mouseBtnCount = (int)MouseButton::Last + 1;
		mPrevMouseState = new u16[mouseBtnCount];
		mCurrMouseState = new u16[mouseBtnCount];

		std::memset(mPrevKeyState, 0, MAX_KEY_CODE * sizeof(u16));
		std::memset(mCurrKeyState, 0, MAX_KEY_CODE * sizeof(u16));
		std::memset(mPrevMouseState, 0, mouseBtnCount * sizeof(u16));
		std::memset(mCurrMouseState, 0, mouseBtnCount * sizeof(u16));
	}

	InputManager::~InputManager()
	{
		delete[] mPrevKeyState; mPrevKeyState = nullptr;
		delete[] mCurrKeyState; mCurrKeyState = nullptr;
		delete[] mPrevMouseState; mPrevMouseState = nullptr;
		delete[] mCurrMouseState; mCurrMouseState = nullptr;
	}

	void InputManager::initByWindowSystem(Window::rs_window* window)
	{
		if (dynamic_cast<Window::rs_window_glfw*>(window)) {
			auto window_glfw = dynamic_cast<Window::rs_window_glfw*>(window);
			this->_callbackKeyId = window_glfw->KeyEvent += [this](KeyCode code,int _, KeyAction action, KeyModifierFlags flags) {
				bool pressed = (action!= KeyAction::Release);
				this->setKeyState(code, pressed, flags);
			};
			this->_callbackMouseBtnId = window_glfw->MouseBtnEvent += [this](MouseButton btn, KeyModifierFlags flags, MouseAction action) {
				bool pressed = action != MouseAction::Release;
				this->setMouseButtonState(btn, pressed,flags);
				};
			this->_callbackMousePosId = window_glfw->CursorEvent += [this](double posX, double posY) {
				int wx = 0;
				int hy = 0;
				RenderSystem::instance()->getWindowSize(wx, hy);
				if (wx == 0 || hy == 0) {
					this->setCursorPos(0,0);
				}
				else {
					this->setCursorPos(posX / wx, posY / hy);
				}std::cout << "Mouse X:" << mCursorX << " Y:" << mCursorY << "\n";
				std::cout << "Mouse DX:" << mCursorX - mPrevCursorX << " DY:" << mCursorY - mPrevCursorY <<"\n";
				};

			this->_callbackWindowFoucsId = window_glfw->FocusEvent += [this](FocusAction action) {
				if (action == FocusAction::Lost) {
					firstMouse = true;
				}
				};
		}
	}

	void InputManager::deinitByWindowSystem(Window::rs_window* window)
	{
		if (!window)return;
		if (dynamic_cast<Window::rs_window_glfw*>(window)) {
			auto window_glfw = dynamic_cast<Window::rs_window_glfw*>(window);
			window_glfw->KeyEvent		-= this->_callbackKeyId;
			window_glfw->MouseBtnEvent	-= this->_callbackMouseBtnId;
			window_glfw->CursorEvent	-= this->_callbackMousePosId;
		}
	}

	void InputManager::beginFrame()
	{
		std::memcpy(mPrevKeyState, mCurrKeyState, MAX_KEY_CODE * sizeof(u16));

		int mouseBtnCount = (int)MouseButton::Last + 1;
		std::memcpy(mPrevMouseState, mCurrMouseState, mouseBtnCount * sizeof(u16));
		if (firstMouse) {
			mPrevCursorX = mCursorX;
			mPrevCursorY = mCursorY;
			firstMouse = false;
		}

		if (this->isKeyDown(KeyCode::RightAlt)) {
			static bool cursorEnable = true;
			cursorEnable = !cursorEnable;
			RenderSystem::instance()->setCursorEnable(cursorEnable);
		}
	}

	Render::KeyModifierFlags InputManager::getKeyModifiers(KeyCode key)
	{
		bool pressed = false;
		KeyModifierFlags flags = 0;
		unpackKeyState(mCurrKeyState[(int)key], flags, pressed);
		return pressed ? flags : 0;
	}

	bool InputManager::isKeyDown(KeyCode key)
	{
		return isKeyStatePressed(mCurrKeyState[(int)key]);
	}

	bool InputManager::isKeyPressed(KeyCode key)
	{
		return !isKeyStatePressed(mPrevKeyState[(int)key]) && isKeyStatePressed(mCurrKeyState[(int)key]);
	}

	bool InputManager::isKeyReleased(KeyCode key)
	{
		return isKeyStatePressed(mPrevKeyState[(int)key]) && !isKeyStatePressed(mCurrKeyState[(int)key]);
	}

	bool InputManager::isKeyHold(KeyCode key)
	{
		return isKeyStatePressed(mPrevKeyState[(int)key]) && isKeyStatePressed(mCurrKeyState[(int)key]);
	}

	bool InputManager::isMouseDown(MouseButton btn)
	{
		return isKeyStatePressed(mCurrMouseState[(int)btn]);
	}

	bool InputManager::isMousePressed(MouseButton btn)
	{
		return !isKeyStatePressed(mPrevMouseState[(int)btn]) && isKeyStatePressed(mCurrMouseState[(int)btn]);
	}

	bool InputManager::isMouseReleased(MouseButton btn)
	{
		return mPrevMouseState[(int)btn] != 0 && mCurrMouseState[(int)btn] == 0;
	}

	bool InputManager::isMouseHold(MouseButton btn)
	{
		return mPrevMouseState[(int)btn] != 0 && mCurrMouseState[(int)btn] != 0;
	}

	void InputManager::getCursorPos(double& x, double& y)
	{
		x = mCursorX;
		y = mCursorY;
	}

	void InputManager::getDeltaCursorPos(double& x, double& y)
	{
		x = mCursorX - mPrevCursorX;
		y = mCursorY - mPrevCursorY;
	}

	Render::KeyModifierFlags InputManager::getMouseModifiers(MouseButton btn)
	{
		bool pressed = false;
		KeyModifierFlags flags = 0;
		unpackKeyState(mCurrMouseState[(int)btn], flags, pressed);
		return pressed ? flags : 0;
	}

	void InputManager::setKeyState(KeyCode code, bool pressed, KeyModifierFlags flag)
	{
		uint16_t keycode = (uint16_t)code;
		if (keycode >= MAX_KEY_CODE) return;
		this->mCurrKeyState[keycode] = packKeyState(flag, pressed);
	}

	void InputManager::setMouseButtonState(MouseButton button, bool pressed, KeyModifierFlags flag)
	{
		if ((int)button > (int)MouseButton::Last) return;
		this->mCurrMouseState[(int)button] = packKeyState(flag, pressed);
	}

	void InputManager::setCursorPos(double x, double y)
	{
		mPrevCursorX = mCursorX;
		mPrevCursorY = mCursorY;
		mCursorX = x;
		mCursorY = y;
	}

	Render::KeyAction InputManager::getMouseButtonState(MouseButton button)
	{
		if (isMousePressed(button))  return KeyAction::Press;
		if (isMouseReleased(button)) return KeyAction::Release;
		if (isMouseHold(button))     return KeyAction::Repeat;
		return KeyAction::None;
	}
}