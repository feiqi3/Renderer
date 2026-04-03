#ifndef RENDER_RESOURCE_WINDOW_H
#define RENDER_RESOURCE_WINDOW_H


namespace Render::Window {

    class rs_window {
    public:
        virtual ~rs_window() {}

        /// 轮询系统事件，返回 false 表示用户请求关闭
        virtual bool pollEvents() = 0;

        /// 获取当前帧缓冲区（像素）大小
        virtual void getFramebufferSize(int& width, int& height) const = 0;

        /// 是否已经请求关闭
        virtual bool shouldClose() const = 0;

        /// 获取原生窗口句柄（GLFWwindow* / HWND / 等）
        virtual void* nativeHandle() const = 0;

        virtual const char* getTitle() const = 0;
        virtual void setTitle(const char* title) = 0;
    
        virtual void setCursorEnable(bool enable) = 0;
    protected:
        bool m_init = false;
    };

}

#endif