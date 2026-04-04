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
				//GLFW will send KeyAction::Pressed once when key is down, and send KeyAction::Repeat if key is hold. We treat both of them as pressed state.
				//And KeyAction::Released will only send once when key is released.
				//So we need maintain a key state by ourselves to support isKeyPressed and isKeyReleased function.
				this->setKeyState(code, pressed, flags);
			};
			this->_callbackMouseBtnId = window_glfw->MouseBtnEvent += [this](MouseButton btn, KeyModifierFlags flags, MouseAction action) {
				bool pressed = action != MouseAction::Release;
				this->setMouseButtonState(btn, pressed,flags);
				};
			this->_callbackMousePosId = window_glfw->CursorEvent += [this](double posX, double posY) {
				this->setCursorPos(posX , posY );
				};

			this->_callbackWindowFoucsId = window_glfw->FocusEvent += [this](FocusAction action) {
				isWindowFoused = (action == FocusAction::Gain);
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

	void InputManager::preUpdate()
	{
		memcpy(mPrevKeyState, mCurrKeyState, MAX_KEY_CODE * sizeof(u16));
		mPrevCursorX = mCursorX;
		mPrevCursorY = mCursorY;
	}

	void InputManager::postUpdate()
	{
		int mouseBtnCount = (int)MouseButton::Last + 1;
		std::memcpy(mPrevMouseState, mCurrMouseState, mouseBtnCount * sizeof(u16));
		if (this->isKeyReleased(KeyCode::RightAlt)) {
			//GLFW will set cursor to a virtual position when 
			//cursor is hidden, which will cause a large delta in cursor position when the window regain focus or cursor move. 
			// To avoid this, we set a flag to ignore the next cursor position update.
			ignoreNextCursor = true;
			static bool cursorEnable = true;
			cursorEnable = !cursorEnable;
			RenderSystem::instance()->setCursorEnable(cursorEnable);
			isCursorUpdated = false;
		}

		if (!isWindowFoused || !isCursorUpdated) {
			// If the window is not focused or cursor is not updated, 
			// Set prev cursor position to current cursor position to avoid large delta in next frame when the window regain focus or cursor move.
			mPrevCursorX = mCursorX;
			mPrevCursorY = mCursorY;
		}
		isCursorUpdated = false;

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
		bool lastFramePressed = isKeyStatePressed(mPrevKeyState[(int)key]);
		if (lastFramePressed) {
			bool thisFramePressed = isKeyStatePressed(mCurrKeyState[(int)key]);
			if (!thisFramePressed) {
				return true;
			}
		}
		return false;
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
		if (isCursorUpdated)return;
		if (ignoreNextCursor) {
			isCursorUpdated = true;
			mPrevCursorX = mCursorX = x;
			mPrevCursorY = mCursorY = y;
			ignoreNextCursor = false;
			return;
		}
		
		isCursorUpdated = true;
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