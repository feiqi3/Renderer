#include "function/InputManager.h"
#include <cstring>
namespace Render {

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
		std::memset(mPrevKeyState, 0, MAX_KEY_CODE * sizeof(u8));
		std::memset(mCurrKeyState, 0, MAX_KEY_CODE * sizeof(u8));
	}
	InputManager::~InputManager()
	{
		delete[] mPrevKeyState;
		mPrevKeyState = nullptr;
		delete[] mCurrKeyState;
		mCurrKeyState = nullptr;
	}
	void InputManager::setKeyState(KeyCode code, KeyModifierFlags flag)
	{
		uint16_t keycode = (uint16_t)code;
		this->mCurrKeyState[keycode] = packKeyState(flag, true);
	}
	void InputManager::setMouseButtonState(int button, bool pressed)
	{
		this->mCurrMouseState[button] = pressed ? 1 : 0;
	}
	void InputManager::getKeyState(KeyCode code, KeyModifierFlags& flag, bool& pressed)
	{
		uint16_t keycode = (uint16_t)code;
		unpackKeyState(this->mCurrKeyState[keycode], flag, pressed);
	}
	void InputManager::getMouseButtonState(int button, bool& pressed)
	{
		mCurrKeyState[button] = pressed ? 1 : 0;
	}
}