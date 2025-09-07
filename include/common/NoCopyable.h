#ifndef RENDER_NO_COPYABLE_H_
#define RENDER_NO_COPYABLE_H_

namespace Render::Common
{
    class NonCopyable {
public:
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable&) = delete;            // 禁止拷贝构造
    NonCopyable& operator=(const NonCopyable&) = delete; // 禁止拷贝赋值
};
}

#endif