#ifndef INPUT_DEF_H_
#define INPUT_DEF_H_

namespace Render {
#define MAX_KEY_CODE  (int)KeyCode::Max

	enum class KeyCode : uint16_t {
		Unknown = 0,

		Space, Apostrophe, Comma, Minus, Period, Slash, Semicolon, Equal,
		LeftBracket, Backslash, RightBracket, GraveAccent, World1, World2,

		Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9,

		A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

		Escape, Enter, Tab, Backspace, Insert, Delete,
		Right, Left, Down, Up, PageUp, PageDown, Home, End,
		CapsLock, ScrollLock, NumLock, PrintScreen, Pause,

		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,
		F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,

		Kp0, Kp1, Kp2, Kp3, Kp4, Kp5, Kp6, Kp7, Kp8, Kp9,
		KpDecimal, KpDivide, KpMultiply, KpSubtract, KpAdd, KpEnter, KpEqual,

		LeftShift, LeftControl, LeftAlt, LeftSuper,
		RightShift, RightControl, RightAlt, RightSuper, Menu,

		Max 
	};

    enum class KeyAction : int {
        None        = 0,
        Press       = 1,
        Release     = 2,
        Repeat      = 3
	};

    enum class MouseButton : uint16_t {
        Button1 = 0,
        Button2 = 1,
        Button3 = 2,
        Button4 = 3,
        Button5 = 4,
        Button6 = 5,
        Button7 = 6,
        Button8 = 7,
        
        Left    = Button1,  
        Right   = Button2,  
        Middle  = Button3,  

        Last    = Button8,
        Max


	};

    enum class MouseAction : uint8_t {
        Press,
        Release
    };

	enum class FocusAction : uint8_t {
		Lost,
		Gain
	};

    using KeyModifierFlags = uint8_t;
    inline KeyModifierFlags KeyModifierFlag_None        = 0;
    inline KeyModifierFlags KeyModifierFlag_Shift       = 0x0001;
    inline KeyModifierFlags KeyModifierFlag_Control     = 0x0002;
    inline KeyModifierFlags KeyModifierFlag_Alt         = 0x0004;
    inline KeyModifierFlags KeyModifierFlag_Super       = 0x0008;
    inline KeyModifierFlags KeyModifierFlag_CapsLock    = 0x0010;
    inline KeyModifierFlags KeyModifierFlag_NumLock     = 0x0020;

}

#endif // INPUT_DEF_H_