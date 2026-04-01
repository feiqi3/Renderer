#include "window/render_resource_window_glfw.h"
#include "GLFW/glfw3.h"
#include <stdexcept>

bool s_glfwInitialized = false;
std::atomic_int s_glfwInstanceNum = 0;

namespace Render::Window
{
    namespace {
    
        uint32_t ToGlfwKeyCode(KeyCode code);
        KeyCode ToKeyCode(uint32_t glfwCode);

        uint32_t ToGlfwKeyMouse(MouseButton btn);
        MouseButton ToMouseButton(uint32_t glfwMus);


        uint32_t ToGlfwKeyCode(KeyCode code)
        {
            return (uint32_t)code;
        }

        KeyCode ToKeyCode(uint32_t glfwCode)
        {
            return (KeyCode)glfwCode;
        }
        uint32_t ToGlfwKeyMouse(MouseButton btn)
        {
            return (uint32_t)btn;
        }
        MouseButton ToMouseButton(uint32_t glfwMus)
        {
            return (MouseButton)glfwMus;
        }
    }

    rs_window_glfw::rs_window_glfw(const char* title, int w, int h)
        : _width(w), _height(h)
    {
        if (!s_glfwInitialized) {
            if (!glfwInit()) {
                m_init = false;
                return;
            }
            // 我们使用 GLFW 仅作窗口和事件，无 OpenGL 上下文
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
    }
    rs_window_glfw::~rs_window_glfw()
    {
        auto win = (GLFWwindow*)(nativeHandle());
        glfwDestroyWindow(win);
        s_glfwInstanceNum--;
        if (s_glfwInstanceNum == 0) {
            glfwTerminate();
            m_init = false;
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
                _win->KeyEvent.dispatch(toKeyCode(key), scancode, actionKey, mods);
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
			MouseButton btn = toMouseButton(button);
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
                _win->MouseBtnEvent.dispatch(btn, mods, actionMus);
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

    uint32_t rs_window_glfw::toGlfwKeyCode(KeyCode code)
    {
        return ToGlfwKeyCode(code);
    }

    KeyCode rs_window_glfw::toKeyCode(uint32_t glfwCode)
    {
        return ToKeyCode(glfwCode);
    }

    uint32_t rs_window_glfw::toGlfwKeyMouse(MouseButton btn)
    {
		return ToGlfwKeyMouse(btn);
    }

    MouseButton rs_window_glfw::toMouseButton(uint32_t glfwMus)
    {
        return ToMouseButton(glfwMus);
    }



}