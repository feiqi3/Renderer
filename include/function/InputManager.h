#ifndef INPUT_MANAGER_H_
#define INPUT_MANAGER_H_
#include "Common/Singleton.h"
#include "Common/CoreDefs.h"
#include "InputDef.h"

namespace Render {
	class InputManager : public Singleton< InputManager >{
	public:
		InputManager();
		~ InputManager();
		KeyAction getKeyAction(KeyCode code);
		bool isKeyPressed(KeyCode, KeyModifierFlags& outModifiers);

		bool isMouseButtonPressed(int button);

		void getCursorPos(double& x, double& y);

		void setKeyState(KeyCode code,KeyModifierFlags flag);
		void setMouseButtonState(int button, bool pressed);

		KeyAction getMouseButtonState(MouseButton button);
	private:
		//||---8 bit---||---8 but---||\\
		//|| Keystate  ||  Modifier ||\\

		u16* mPrevKeyState		= nullptr;
		u16* mCurrKeyState		= nullptr;
		u8* mPrevMouseState		= nullptr;
		u8* mCurrMouseState		= nullptr;
	};
}

#endif