#ifndef INPUT_MANAGER_H_
#define INPUT_MANAGER_H_
#include <queue>
#include "Common/Singleton.h"
#include "Common/CoreDefs.h"
#include "InputDef.h"
#include "window/EventDispatcher.h"
namespace Render {
	namespace Window {
		class rs_window;
	}



	class InputManager : public Singleton< InputManager > {
	public:
		struct KeyState {
			uint8_t			 isPressed = 0;
			KeyModifierFlags modifier = 0;
		};
	public:
		InputManager();
		~InputManager();
		void initByWindowSystem(Window::rs_window* window);
		void deinitByWindowSystem(Window::rs_window* window);

		void preUpdate();
		void postUpdate();

		KeyModifierFlags getKeyModifiers(KeyCode key);
		bool isKeyDown(KeyCode key);
		bool isKeyPressed(KeyCode key);
		bool isKeyReleased(KeyCode key);
		bool isKeyHold(KeyCode key);

		KeyModifierFlags getMouseModifiers(MouseButton btn);
		bool isMouseDown(MouseButton btn);
		bool isMousePressed(MouseButton btn);
		bool isMouseReleased(MouseButton btn);
		bool isMouseHold(MouseButton btn);

		void getCursorPos(double& x, double& y);
		void getDeltaCursorPos(double& x, double& y);

		void consumeKey(KeyCode key);
		void consumeMouse(MouseButton btn);

		uint32_t peekChar();
		uint32_t consumeChar();

		KeyAction getMouseButtonState(MouseButton button);
	private:
		bool isBtnConsumed(MouseButton button)const;
		bool isKeyConsumed(KeyCode button)const;
		void setKeyState(KeyCode code, bool pressed, KeyModifierFlags flag);
		void setMouseButtonState(MouseButton button, bool pressed, KeyModifierFlags flag);
		void setCursorPos(double x, double y);

	private:

		KeyState* mPrevKeyState = nullptr;
		KeyState* mCurrKeyState = nullptr;
		KeyState* mPrevMouseState = nullptr;
		KeyState* mCurrMouseState = nullptr;
		u8*		  mIsKeyConsumed = nullptr;
		u8*		  mIsMouseConsumed = nullptr;
		std::queue<uint32_t> mCharQueueCurFrame;

		double mCursorX = 0.0;
		double mCursorY = 0.0;
		double mPrevCursorX = 0.0;
		double mPrevCursorY = 0.0;
		bool isWindowFoused = true;
		bool isCursorUpdated = false;
		bool ignoreNextCursor = false;
	private:
		Window::CallbackID _callbackKeyId		= Window::INVALID_CALLBACK_ID;
		Window::CallbackID _callbackMouseBtnId	= Window::INVALID_CALLBACK_ID;
		Window::CallbackID _callbackMousePosId	= Window::INVALID_CALLBACK_ID;
		Window::CallbackID _callbackWindowFoucsId = Window::INVALID_CALLBACK_ID;
		Window::CallbackID _callbackCharInputId	= Window::INVALID_CALLBACK_ID;
	};
}

#endif