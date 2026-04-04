#ifndef INPUT_MANAGER_H_
#define INPUT_MANAGER_H_
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


		KeyAction getMouseButtonState(MouseButton button);
	private:

		void setKeyState(KeyCode code, bool pressed, KeyModifierFlags flag);
		void setMouseButtonState(MouseButton button, bool pressed, KeyModifierFlags flag);
		void setCursorPos(double x, double y);

	private:
		//||---8 bit---||---8 bit---||\\
        //|| Keystate  ||  Modifier ||\\

		u16* mPrevKeyState = nullptr;
		u16* mCurrKeyState = nullptr;
		u16* mPrevMouseState = nullptr;
		u16* mCurrMouseState = nullptr;

		double mCursorX = 0.0;
		double mCursorY = 0.0;
		double mPrevCursorX = 0.0;
		double mPrevCursorY = 0.0;
		bool isWindowFoused = false;
		bool isCursorUpdated = false;
		bool ignoreNextCursor = false;
	private:
		Window::CallbackID _callbackKeyId		= Window::INVALID_CALLBACK_ID;
		Window::CallbackID _callbackMouseBtnId	= Window::INVALID_CALLBACK_ID;
		Window::CallbackID _callbackMousePosId	= Window::INVALID_CALLBACK_ID;
		Window::CallbackID _callbackWindowFoucsId = Window::INVALID_CALLBACK_ID;
	};
}

#endif