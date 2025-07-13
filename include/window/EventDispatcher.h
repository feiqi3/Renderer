// EventDispatcher.h
#ifndef EVENT_DISPATCHER_H
#define EVENT_DISPATCHER_H
#pragma once

#include <functional>
#include <map>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cstdint>

namespace Render::Window {

    // 回调标识
    using CallbackID = uint64_t;

    // 泛型事件分发器
    template<typename... Args>
    class EventDispatcher {
    public:

        void dispatch(Args... args) {
            std::lock_guard<std::mutex> lk(_mtx);
            for (auto const& [id, fn] : _callbacks) {
                fn(args...);
            }
        }

        CallbackID operator +=(std::function<void(Args...)> const& fn) {
            add(fn);
        }

        void operator -=(CallbackID id) {
            remove(id);
        }
    private:
        std::map<CallbackID, std::function<void(Args...)>> _callbacks;

        CallbackID add(std::function<void(Args...)> const& fn) {
            std::lock_guard<std::mutex> lk(_mtx);
            CallbackID id = _nextID++;
            _callbacks.insert({ id, fn });
            return id;
        }

        void remove(CallbackID id) {
            std::lock_guard<std::mutex> lk(_mtx);
            _callbacks.erase(id);
        }

        CallbackID _nextID = 1;
        std::mutex _mtx;
    };
}

#endif