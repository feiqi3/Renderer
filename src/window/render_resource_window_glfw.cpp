#include "window/render_resource_window_glfw.h"
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <array>
#include "function/InputDef.h"

bool s_glfwInitialized = false;
std::atomic_int s_glfwInstanceNum = 0;

namespace Render {
	namespace {
		KeyCode ToKeyCode(uint32_t glfwCode);

		MouseButton ToMouseButton(uint32_t glfwMus);
		KeyModifierFlags ToModifierFlags(int mod);

		KeyCode ToKeyCode(uint32_t glfwCode)
		{
            if (glfwCode >= GLFW_KEY_LAST + 1)return KeyCode::Unknown;

			static const std::array<KeyCode, GLFW_KEY_LAST + 1> GlfwToKeyCodeMap= []() {
                std::array<KeyCode, GLFW_KEY_LAST + 1> map;
                map.fill(KeyCode::Unknown);
				map[GLFW_KEY_SPACE] = KeyCode::Space;
				map[GLFW_KEY_APOSTROPHE] = KeyCode::Apostrophe;
				map[GLFW_KEY_COMMA] = KeyCode::Comma;
				map[GLFW_KEY_MINUS] = KeyCode::Minus;
				map[GLFW_KEY_PERIOD] = KeyCode::Period;
				map[GLFW_KEY_SLASH] = KeyCode::Slash;
				map[GLFW_KEY_SEMICOLON] = KeyCode::Semicolon;
				map[GLFW_KEY_EQUAL] = KeyCode::Equal;

				map[GLFW_KEY_0] = KeyCode::Key0; map[GLFW_KEY_1] = KeyCode::Key1; map[GLFW_KEY_2] = KeyCode::Key2;
				map[GLFW_KEY_3] = KeyCode::Key3; map[GLFW_KEY_4] = KeyCode::Key4; map[GLFW_KEY_5] = KeyCode::Key5;
				map[GLFW_KEY_6] = KeyCode::Key6; map[GLFW_KEY_7] = KeyCode::Key7; map[GLFW_KEY_8] = KeyCode::Key8;
				map[GLFW_KEY_9] = KeyCode::Key9;

				map[GLFW_KEY_A] = KeyCode::A; map[GLFW_KEY_B] = KeyCode::B; map[GLFW_KEY_C] = KeyCode::C;
				map[GLFW_KEY_D] = KeyCode::D; map[GLFW_KEY_E] = KeyCode::E; map[GLFW_KEY_F] = KeyCode::F;
				map[GLFW_KEY_G] = KeyCode::G; map[GLFW_KEY_H] = KeyCode::H; map[GLFW_KEY_I] = KeyCode::I;
				map[GLFW_KEY_J] = KeyCode::J; map[GLFW_KEY_K] = KeyCode::K; map[GLFW_KEY_L] = KeyCode::L;
				map[GLFW_KEY_M] = KeyCode::M; map[GLFW_KEY_N] = KeyCode::N; map[GLFW_KEY_O] = KeyCode::O;
				map[GLFW_KEY_P] = KeyCode::P; map[GLFW_KEY_Q] = KeyCode::Q; map[GLFW_KEY_R] = KeyCode::R;
				map[GLFW_KEY_S] = KeyCode::S; map[GLFW_KEY_T] = KeyCode::T; map[GLFW_KEY_U] = KeyCode::U;
				map[GLFW_KEY_V] = KeyCode::V; map[GLFW_KEY_W] = KeyCode::W; map[GLFW_KEY_X] = KeyCode::X;
				map[GLFW_KEY_Y] = KeyCode::Y; map[GLFW_KEY_Z] = KeyCode::Z;

				map[GLFW_KEY_LEFT_BRACKET] = KeyCode::LeftBracket;
				map[GLFW_KEY_BACKSLASH] = KeyCode::Backslash;
				map[GLFW_KEY_RIGHT_BRACKET] = KeyCode::RightBracket;
				map[GLFW_KEY_GRAVE_ACCENT] = KeyCode::GraveAccent;
				map[GLFW_KEY_WORLD_1] = KeyCode::World1;
				map[GLFW_KEY_WORLD_2] = KeyCode::World2;

				map[GLFW_KEY_ESCAPE] = KeyCode::Escape;
				map[GLFW_KEY_ENTER] = KeyCode::Enter;
				map[GLFW_KEY_TAB] = KeyCode::Tab;
				map[GLFW_KEY_BACKSPACE] = KeyCode::Backspace;
				map[GLFW_KEY_INSERT] = KeyCode::Insert;
				map[GLFW_KEY_DELETE] = KeyCode::Delete;
				map[GLFW_KEY_RIGHT] = KeyCode::Right;
				map[GLFW_KEY_LEFT] = KeyCode::Left;
				map[GLFW_KEY_DOWN] = KeyCode::Down;
				map[GLFW_KEY_UP] = KeyCode::Up;
				map[GLFW_KEY_PAGE_UP] = KeyCode::PageUp;
				map[GLFW_KEY_PAGE_DOWN] = KeyCode::PageDown;
				map[GLFW_KEY_HOME] = KeyCode::Home;
				map[GLFW_KEY_END] = KeyCode::End;
				map[GLFW_KEY_CAPS_LOCK] = KeyCode::CapsLock;
				map[GLFW_KEY_SCROLL_LOCK] = KeyCode::ScrollLock;
				map[GLFW_KEY_NUM_LOCK] = KeyCode::NumLock;
				map[GLFW_KEY_PRINT_SCREEN] = KeyCode::PrintScreen;
				map[GLFW_KEY_PAUSE] = KeyCode::Pause;

				map[GLFW_KEY_F1] = KeyCode::F1;  map[GLFW_KEY_F2] = KeyCode::F2;  map[GLFW_KEY_F3] = KeyCode::F3;
				map[GLFW_KEY_F4] = KeyCode::F4;  map[GLFW_KEY_F5] = KeyCode::F5;  map[GLFW_KEY_F6] = KeyCode::F6;
				map[GLFW_KEY_F7] = KeyCode::F7;  map[GLFW_KEY_F8] = KeyCode::F8;  map[GLFW_KEY_F9] = KeyCode::F9;
				map[GLFW_KEY_F10] = KeyCode::F10; map[GLFW_KEY_F11] = KeyCode::F11; map[GLFW_KEY_F12] = KeyCode::F12;
				map[GLFW_KEY_F13] = KeyCode::F13; map[GLFW_KEY_F14] = KeyCode::F14; map[GLFW_KEY_F15] = KeyCode::F15;
				map[GLFW_KEY_F16] = KeyCode::F16; map[GLFW_KEY_F17] = KeyCode::F17; map[GLFW_KEY_F18] = KeyCode::F18;
				map[GLFW_KEY_F19] = KeyCode::F19; map[GLFW_KEY_F20] = KeyCode::F20; map[GLFW_KEY_F21] = KeyCode::F21;
				map[GLFW_KEY_F22] = KeyCode::F22; map[GLFW_KEY_F23] = KeyCode::F23; map[GLFW_KEY_F24] = KeyCode::F24;
				map[GLFW_KEY_F25] = KeyCode::F25;

				map[GLFW_KEY_KP_0] = KeyCode::Kp0; map[GLFW_KEY_KP_1] = KeyCode::Kp1; map[GLFW_KEY_KP_2] = KeyCode::Kp2;
				map[GLFW_KEY_KP_3] = KeyCode::Kp3; map[GLFW_KEY_KP_4] = KeyCode::Kp4; map[GLFW_KEY_KP_5] = KeyCode::Kp5;
				map[GLFW_KEY_KP_6] = KeyCode::Kp6; map[GLFW_KEY_KP_7] = KeyCode::Kp7; map[GLFW_KEY_KP_8] = KeyCode::Kp8;
				map[GLFW_KEY_KP_9] = KeyCode::Kp9;
				map[GLFW_KEY_KP_DECIMAL] = KeyCode::KpDecimal;
				map[GLFW_KEY_KP_DIVIDE] = KeyCode::KpDivide;
				map[GLFW_KEY_KP_MULTIPLY] = KeyCode::KpMultiply;
				map[GLFW_KEY_KP_SUBTRACT] = KeyCode::KpSubtract;
				map[GLFW_KEY_KP_ADD] = KeyCode::KpAdd;
				map[GLFW_KEY_KP_ENTER] = KeyCode::KpEnter;
				map[GLFW_KEY_KP_EQUAL] = KeyCode::KpEqual;

				map[GLFW_KEY_LEFT_SHIFT] = KeyCode::LeftShift;
				map[GLFW_KEY_LEFT_CONTROL] = KeyCode::LeftControl;
				map[GLFW_KEY_LEFT_ALT] = KeyCode::LeftAlt;
				map[GLFW_KEY_LEFT_SUPER] = KeyCode::LeftSuper;
				map[GLFW_KEY_RIGHT_SHIFT] = KeyCode::RightShift;
				map[GLFW_KEY_RIGHT_CONTROL] = KeyCode::RightControl;
				map[GLFW_KEY_RIGHT_ALT] = KeyCode::RightAlt;
				map[GLFW_KEY_RIGHT_SUPER] = KeyCode::RightSuper;
				map[GLFW_KEY_MENU] = KeyCode::Menu;

				return map;
				}();


			return GlfwToKeyCodeMap[glfwCode];
		}

		MouseButton ToMouseButton(uint32_t glfwMus)
		{
            if (glfwMus >= GLFW_MOUSE_BUTTON_LAST + 1)return MouseButton::Max;
			static const std::array<Render::MouseButton, GLFW_MOUSE_BUTTON_LAST + 1> GlfwToMouseButtonMap= []() {
                std::array<Render::MouseButton, GLFW_MOUSE_BUTTON_LAST + 1> map;
				map[GLFW_MOUSE_BUTTON_1] = Render::MouseButton::Button1; // 0
				map[GLFW_MOUSE_BUTTON_2] = Render::MouseButton::Button2; // 1
				map[GLFW_MOUSE_BUTTON_3] = Render::MouseButton::Button3; // 2[cite: 1]
				map[GLFW_MOUSE_BUTTON_4] = Render::MouseButton::Button4; // 3[cite: 1]
				map[GLFW_MOUSE_BUTTON_5] = Render::MouseButton::Button5; // 4[cite: 1]
				map[GLFW_MOUSE_BUTTON_6] = Render::MouseButton::Button6; // 5[cite: 1]
				map[GLFW_MOUSE_BUTTON_7] = Render::MouseButton::Button7; // 6[cite: 1]
				map[GLFW_MOUSE_BUTTON_8] = Render::MouseButton::Button8; // 7[cite: 1]

				return map;
				}();

			return GlfwToMouseButtonMap[glfwMus];
		}

		KeyModifierFlags ToModifierFlags(int mod) {
			KeyModifierFlags flags = KeyModifierFlag_None;

			if (mod & GLFW_MOD_SHIFT)     flags |= KeyModifierFlag_Shift;
			if (mod & GLFW_MOD_CONTROL)   flags |= KeyModifierFlag_Control;
			if (mod & GLFW_MOD_ALT)       flags |= KeyModifierFlag_Alt;
			if (mod & GLFW_MOD_SUPER)     flags |= KeyModifierFlag_Super;
			if (mod & GLFW_MOD_CAPS_LOCK) flags |= KeyModifierFlag_CapsLock;
			if (mod & GLFW_MOD_NUM_LOCK)  flags |= KeyModifierFlag_NumLock;

			return flags;
		}
	}

}

namespace Render::Window
{

    rs_window_glfw::rs_window_glfw(const char* title, int w, int h)
        : _width(w), _height(h)
    {
        if (!s_glfwInitialized) {
            if (!glfwInit()) {
                m_init = false;
                return;
            }
        }
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            _window = glfwCreateWindow(w, h, title, nullptr, nullptr);
            if (!_window) {
                m_init = false;
                return;
            }
            s_glfwInitialized = true;
            s_glfwInstanceNum++;
            initCallback();
    }
    rs_window_glfw::~rs_window_glfw()
    {
        auto win = (GLFWwindow*)(nativeHandle());
        glfwDestroyWindow(win);
		m_init = false;
		s_glfwInstanceNum--;
        if (s_glfwInstanceNum == 0) {
            glfwTerminate();
			s_glfwInitialized = false;
        }
    }
    bool rs_window_glfw::pollEvents()
    {
        glfwPollEvents();
        return true;
    }
    void rs_window_glfw::getFramebufferSize(int& w, int& h) const
    {
        glfwGetWindowSize(_window, &w, &h);
    }
    bool rs_window_glfw::shouldClose() const
    {
        return glfwWindowShouldClose(_window)> 0;
    }
    void* rs_window_glfw::nativeHandle() const
    {
        return this->_window;
    }
    const char* Render::Window::rs_window_glfw::getTitle() const
    {
        return glfwGetWindowTitle((GLFWwindow*)(nativeHandle()));
    }
    void rs_window_glfw::setTitle(const char* title)
    {
        glfwSetWindowTitle((GLFWwindow*)(nativeHandle()), title);
    }
    void rs_window_glfw::initCallback()
    {
        auto win = (GLFWwindow*)(nativeHandle());
        glfwSetWindowUserPointer(win, this);

        glfwSetWindowSizeCallback(win, [](GLFWwindow* window, int width, int height) {
            rs_window_glfw* _win = (rs_window_glfw*)glfwGetWindowUserPointer(window);
            if (_win) {
                _win->ResizeEvent.dispatch(width, height);
            }
        });

        glfwSetWindowCloseCallback(win, [](GLFWwindow* window) {
            rs_window_glfw* _win = (rs_window_glfw*)glfwGetWindowUserPointer(window);
            if (_win) {
                _win->CloseEvent.dispatch();
            }
        });

        glfwSetKeyCallback(win, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            rs_window_glfw* _win = (rs_window_glfw*)glfwGetWindowUserPointer(window);
            KeyAction actionKey;
            switch (action)
            {
            case GLFW_RELEASE:
                actionKey = KeyAction::Release;
                break;
            case GLFW_REPEAT:
                actionKey = KeyAction::Repeat;
                break;
            case GLFW_PRESS:
            default:
                actionKey = KeyAction::Press;
                break;
            }

            if (_win) {
                _win->KeyEvent.dispatch(ToKeyCode(key), scancode, actionKey, ToModifierFlags(mods));
            }
        });

        glfwSetCharCallback(win, [](GLFWwindow* window, unsigned int codepoint) {
            rs_window_glfw* _win = (rs_window_glfw*)glfwGetWindowUserPointer(window);
            if (_win) {
                _win->CharEvent.dispatch(codepoint);
            }
        });

        glfwSetMouseButtonCallback(win, [](GLFWwindow* window, int button, int action, int mods) {
            rs_window_glfw* _win = (rs_window_glfw*)glfwGetWindowUserPointer(window);
            MouseAction actionMus;
            switch (action)
            {
            case GLFW_RELEASE:
                actionMus = MouseAction::Release;
                break;
            case GLFW_PRESS:
            default:
                actionMus = MouseAction::Press;
                break;
            }

            if (_win) {
                _win->MouseBtnEvent.dispatch(ToMouseButton(button),ToModifierFlags(mods), actionMus);
            }
        });

        glfwSetCursorPosCallback(win, [](GLFWwindow* window, double xpos, double ypos) {
            rs_window_glfw* _win = (rs_window_glfw*)glfwGetWindowUserPointer(window);
            if (_win) {
                _win->CursorEvent.dispatch(xpos,ypos);
            }
        });

        glfwSetScrollCallback(win, [](GLFWwindow* window, double xoffset, double yoffset) {
            rs_window_glfw* _win = (rs_window_glfw*)glfwGetWindowUserPointer(window);
            if (_win) {
                _win->ScrollEvent.dispatch(xoffset, yoffset);
            }
        });

        glfwSetWindowFocusCallback(win, [](GLFWwindow* window, int focused) {
            rs_window_glfw* _win = (rs_window_glfw*)glfwGetWindowUserPointer(window);
            FocusAction action = focused == GLFW_TRUE ? FocusAction::Gain : FocusAction::Lost;
            if (_win) {
                _win->FocusEvent.dispatch(action);
            }
        });
    }

    KeyCode rs_window_glfw::toKeyCode(uint32_t glfwCode)
    {
        return ToKeyCode(glfwCode);
    }

    MouseButton rs_window_glfw::toMouseButton(uint32_t glfwMus)
    {
        return ToMouseButton(glfwMus);
    }


	void rs_window_glfw::setCursorEnable(bool enable)
	{
        glfwSetInputMode(_window, GLFW_CURSOR, enable ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}

}