#pragma once

namespace Genesis {
    enum LoggingLevel : uint8_t {
        GN_DEBUG = 3U,
        GN_INFO = 4U,
        GN_WARNING = 5U,
        GN_ERROR = 6U,
        GN_CRITICAL = 7U
    };

    class Logger {
        public:
            Logger();
            ~Logger();

            static bool init(std::string applicationName);
            static void setCoreLogLevel(LoggingLevel newLoggingLevel);
            static void setClientLogLevel(LoggingLevel newLoggingLevel);

            inline static std::shared_ptr<quill::Logger> getCoreLogger() { return s_coreLogger; }
            inline static std::shared_ptr<quill::Logger> getClientLogger() { return s_clientLogger; }

        private:
            static std::shared_ptr<quill::Logger> s_coreLogger;
            static std::shared_ptr<quill::Logger> s_clientLogger;
    };
}  // namespace Genesis

#define GN_CORE_CRITICAL(...) QUILL_LOG_CRITICAL(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)
#define GN_CORE_ERROR(...) QUILL_LOG_ERROR(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)
#define GN_CORE_WARNING(...) QUILL_LOG_WARNING(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)
#define GN_CORE_INFO(...) QUILL_LOG_INFO(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)
#define GN_CORE_DEBUG(...) QUILL_LOG_DEBUG(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)

#define GN_CLIENT_CRITICAL(...) QUILL_LOG_CRITICAL(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
#define GN_CLIENT_ERROR(...) QUILL_LOG_ERROR(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
#define GN_CLIENT_WARNING(...) QUILL_LOG_WARNING(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
#define GN_CLIENT_INFO(...) QUILL_LOG_INFO(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
#define GN_CLIENT_DEBUG(...) QUILL_LOG_DEBUG(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
