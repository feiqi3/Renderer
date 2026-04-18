#ifndef RENDER_LOG_H
#define RENDER_LOG_H

#include <iostream>
#include <mutex>
#include <string>
#include <cassert>
namespace Render {
    namespace Log {
        enum class Level { INFO, WARN, ERR };

        inline std::mutex logMutex;

        inline void log(Level level, const std::string& message) {
            std::lock_guard<std::mutex> lock(logMutex);
            switch (level) {
            case Level::INFO:
                std::cerr << "[INFO] ";
                break;
            case Level::WARN:
                std::cerr << "[WARN] ";
                break;
            case Level::ERR:
                std::cerr << "[ERROR] ";
                break;
            }
            std::cerr << message << std::endl;
        }

        inline void info(const std::string& message) {
            log(Level::INFO, message);
        }

        inline void warn(const std::string& message) {
            log(Level::WARN, message);
        }

        inline void error(const std::string& message) {
            log(Level::ERR, message);
            //assert(false);
        }
    }
}
#endif // RENDER_LOG_H
