#ifndef RENDER_RESOURCE_WINDOW_GLFW_H
#define RENDER_RESOURCE_WINDOW_GLFW_H
#include "render_resource_window.h"
#include "EventDispatcher.h"

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

    enum class KeyAction : uint8_t {
        Press,  
        Release, 
        Repeat    
    };

    /// 通用鼠标按键动作
    enum class MouseAction : uint8_t {
        Press,    
        Release   
    };

    /// 窗口焦点动作
    enum class FocusAction : uint8_t {
        Lost,     
        Gain      
    };

    class rs_window_glfw : public rs_window {
    public:
        rs_window_glfw(const char* title, int w, int h);

        ~rs_window_glfw();

        bool pollEvents() override;

        void getFramebufferSize(int& w, int& h) const override;

        bool shouldClose() const override;

        void* nativeHandle() const override;

        virtual const char* getTitle() const override;
        virtual void setTitle(const char* title) override;

    public:
        EventDispatcher<int, int>           ResizeEvent;
        EventDispatcher<>                   CloseEvent;
        //key ,scancode ,action ,mods
        EventDispatcher<int, int, KeyAction, int> KeyEvent;
        EventDispatcher<unsigned int>       CharEvent;
        EventDispatcher<int, int, MouseAction>      MouseBtnEvent;
        EventDispatcher<double, double>     CursorEvent;
        EventDispatcher<double, double>     ScrollEvent;
        EventDispatcher<FocusAction>                FocusEvent;
    private:
        void initCallback();

    private:
        struct GLFWwindow* _window = nullptr;
        int         _width;
        int         _height;
        bool        _resized = false;


    };

}

#endif