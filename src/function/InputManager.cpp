#include "function/InputManager.h"
#include <cstring>
#include <iostream>
#include "Renderer/RenderSystem.h" 
#include "window/render_resource_window_glfw.h"
namespace Render {
	bool isKeyStatePressed(InputManager::KeyState state) {
		//Pressed and not marked as consumed
		return state.isPressed > 0 ;
	}

	InputManager::KeyState packKeyState(KeyModifierFlags flag, bool pressed) {
		InputManager::KeyState state{};
		state.modifier = flag;
		state.isPressed = pressed;
		return state;
	}

	void unpackKeyState(InputManager::KeyState keyState, KeyModifierFlags& flag, bool& pressed) {

		flag = keyState.modifier;
		pressed = keyState.isPressed;
	}

	Render::InputManager::InputManager()
	{
		mPrevKeyState = new InputManager::KeyState[MAX_KEY_CODE];
		mCurrKeyState = new InputManager::KeyState[MAX_KEY_CODE];

		int mouseBtnCount = (int)MouseButton::Max;
		mPrevMouseState = new InputManager::KeyState[mouseBtnCount];
		mCurrMouseState = new InputManager::KeyState[mouseBtnCount];
		mIsKeyConsumed = new u8[(int)MAX_KEY_CODE];
		mIsMouseConsumed = new u8[(int)mouseBtnCount];
		std::memset(mPrevKeyState, 0, MAX_KEY_CODE * sizeof(InputManager::KeyState));
		std::memset(mCurrKeyState, 0, MAX_KEY_CODE * sizeof(InputManager::KeyState));
		std::memset(mPrevMouseState, 0, mouseBtnCount * sizeof(InputManager::KeyState));
		std::memset(mCurrMouseState, 0, mouseBtnCount * sizeof(InputManager::KeyState));

		std::memset(mIsKeyConsumed, 0, MAX_KEY_CODE * sizeof(u8));
		std::memset(mIsMouseConsumed, 0, mouseBtnCount * sizeof(u8));
	}

	InputManager::~InputManager()
	{
		delete[] mPrevKeyState; mPrevKeyState = nullptr;
		delete[] mCurrKeyState; mCurrKeyState = nullptr;
		delete[] mPrevMouseState; mPrevMouseState = nullptr;
		delete[] mCurrMouseState; mCurrMouseState = nullptr;
		delete[] mIsKeyConsumed; mIsKeyConsumed = nullptr;
		delete[] mIsMouseConsumed; mIsMouseConsumed = nullptr;
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

			this->_callbackCharInputId = window_glfw->CharEvent += [this](uint32_t code) {
				this->mCharQueueCurFrame.push(code);
			};

			this->_callbackScrollId = window_glfw->ScrollEvent += [this](double x, double y) {
				this->mScrollX = x;
				this->mScrollY = y;
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
			window_glfw->CharEvent		-= this->_callbackCharInputId;
			window_glfw->ScrollEvent	-= this->_callbackScrollId;
		}
	}

	void InputManager::preUpdate()
	{
		memcpy(mPrevKeyState, mCurrKeyState, MAX_KEY_CODE * sizeof(InputManager::KeyState));
		int mouseBtnCount = (int)MouseButton::Max;
		memcpy(mPrevMouseState, mCurrMouseState, mouseBtnCount * sizeof(InputManager::KeyState));
		std::memset(mIsKeyConsumed, 0, MAX_KEY_CODE * sizeof(u8));
		std::memset(mIsMouseConsumed, 0, (int)MouseButton::Max * sizeof(u8));
		std::queue<uint32_t> empty;
		mCharQueueCurFrame.swap(empty);
		mConsumeMouseMove = false;
		mPrevCursorX = mCursorX;
		mPrevCursorY = mCursorY;

		mScrollX = 0.;
		mScrollY = 0.;
	}

	void InputManager::postUpdate()
	{

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
		if (mIsKeyConsumed[(int)key] > 0)return 0;
		bool pressed = false;
		KeyModifierFlags flags = 0;
		unpackKeyState(mCurrKeyState[(int)key], flags, pressed);
		return pressed ? flags : 0;
	}

	bool InputManager::isKeyDown(KeyCode key)
	{
		if (mIsKeyConsumed[(int)key] > 0)return false;

		return isKeyStatePressed(mCurrKeyState[(int)key]);
	}

	bool InputManager::isKeyPressed(KeyCode key)
	{
		if (mIsKeyConsumed[(int)key] > 0)return false;
		return !isKeyStatePressed(mPrevKeyState[(int)key]) && isKeyStatePressed(mCurrKeyState[(int)key]);
	}

	bool InputManager::isKeyReleased(KeyCode key)
	{
		if (mIsKeyConsumed[(int)key] > 0)return false;

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
		if (mIsKeyConsumed[(int)key] > 0)return false;
		return isKeyStatePressed(mPrevKeyState[(int)key]) && isKeyStatePressed(mCurrKeyState[(int)key]);
	}

	bool InputManager::isMouseDown(MouseButton btn)
	{
		if (mIsMouseConsumed[(int)btn])return false;
		return isKeyStatePressed(mCurrMouseState[(int)btn]);
	}

	bool InputManager::isMousePressed(MouseButton btn)
	{
		if (mIsMouseConsumed[(int)btn])return false;
		return !isKeyStatePressed(mPrevMouseState[(int)btn]) && isKeyStatePressed(mCurrMouseState[(int)btn]);
	}

	bool InputManager::isMouseReleased(MouseButton btn)
	{
		if (mIsMouseConsumed[(int)btn])return false;
		return  isKeyStatePressed(mPrevMouseState[(int)btn]) && !isKeyStatePressed(mCurrMouseState[(int)btn]);
	}

	bool InputManager::isMouseHold(MouseButton btn)
	{
		if (mIsMouseConsumed[(int)btn])return false;
		return isKeyStatePressed(mPrevMouseState[(int)btn]) && isKeyStatePressed(mCurrMouseState[(int)btn]);
	}

	void InputManager::getCursorPos(double& x, double& y)
	{
		x = mCursorX;
		y = mCursorY;
	}

	void InputManager::getDeltaCursorPos(double& x, double& y)
	{
		if (mConsumeMouseMove) {
			x = 0.;
			y = 0.;
			return;
		}
		x = mCursorX - mPrevCursorX;
		y = mCursorY - mPrevCursorY;
	}

	void InputManager::getMouseScroll(double& x, double& y)
	{
		x = mScrollX;
		y = mScrollY;
	}

	void InputManager::consumeKey(KeyCode key)
	{
		mIsKeyConsumed[(int)key] = 1;
		//mCurrKeyState[(int)key] = packKeyState(0, false);
	}

	void InputManager::consumeMouse(MouseButton btn)
	{
		mIsMouseConsumed[(int)btn] = 1;
		//mCurrMouseState[(int)btn] = packKeyState(0, false);
	}

	void InputManager::consumeMouseMove()
	{
		mConsumeMouseMove = true;
	}

	void InputManager::consumeScroll()
	{
		mScrollX = 0;
		mScrollY = 0.;
	}

	uint32_t InputManager::peekChar()
	{
		if (mCharQueueCurFrame.empty())return 0;
		return mCharQueueCurFrame.front();
	}

	uint32_t InputManager::consumeChar()
	{
		if (mCharQueueCurFrame.empty())return 0;

		auto code = mCharQueueCurFrame.front();
		mCharQueueCurFrame.pop();
		return code;
	}

	Render::KeyModifierFlags InputManager::getMouseModifiers(MouseButton btn)
	{
		bool pressed = false;
		KeyModifierFlags flags = 0;
		unpackKeyState(mCurrMouseState[(int)btn], flags, pressed);
		return pressed ? flags : 0;
	}

	bool InputManager::isBtnConsumed(MouseButton button) const
	{
		return mIsMouseConsumed[(int)button];
	}



	bool InputManager::isKeyConsumed(KeyCode key) const
	{
		return mIsKeyConsumed[(int)key];
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

	void InputManager::setMouseScroll(float x, float y)
	{
		mScrollX = x;
		mScrollY = y;
	}

	Render::KeyAction InputManager::getMouseButtonState(MouseButton button)
	{
		if (isMousePressed(button))  return KeyAction::Press;
		if (isMouseReleased(button)) return KeyAction::Release;
		if (isMouseHold(button))     return KeyAction::Repeat;
		return KeyAction::None;
	}
}