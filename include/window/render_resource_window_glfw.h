#ifndef RENDER_RESOURCE_WINDOW_GLFW_H
#define RENDER_RESOURCE_WINDOW_GLFW_H
#include "render_resource_window.h"
#include "EventDispatcher.h"
#include "function/InputDef.h"
struct GLFWwindow;

namespace Render::Window {

    enum class WindowEvent {
        FramebufferResized,
        WindowClose,
        Key,
        Char,
        MouseButton,
        CursorPos,
        Scroll,
        WindowFocus,
    };

    class rs_window_glfw : public rs_window {
    public:
        rs_window_glfw(const char* title, int w, int h);

        ~rs_window_glfw();

        bool pollEvents() override;

        void getFramebufferSize(int& w, int& h) const override;

        bool shouldClose() const override;

        void* nativeHandle() const override;
		virtual void setCursorEnable(bool enable) override;

        virtual const char* getTitle() const override;
        virtual void setTitle(const char* title) override;

    public:
        EventDispatcher<int, int>           ResizeEvent;
        EventDispatcher<>                   CloseEvent;
        //key ,scancode ,action ,mods
        EventDispatcher<KeyCode, int, KeyAction, KeyModifierFlags> KeyEvent;
        EventDispatcher<unsigned int>       CharEvent;
        EventDispatcher<MouseButton, KeyModifierFlags, MouseAction>      MouseBtnEvent;
        EventDispatcher<double, double>     CursorEvent;
        EventDispatcher<double, double>     ScrollEvent;
        EventDispatcher<FocusAction>                FocusEvent;
    private:
        void initCallback();
		uint32_t toGlfwKeyCode(KeyCode code);
		KeyCode toKeyCode(uint32_t glfwCode);

        uint32_t toGlfwKeyMouse(MouseButton btn);
        MouseButton toMouseButton(uint32_t glfwMus);

    private:
        struct GLFWwindow* _window = nullptr;
        int         _width;
        int         _height;
        bool        _resized = false;
        bool        _firstMouse = true;

    };

}

#endif