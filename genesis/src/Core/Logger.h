#pragma once

namespace Genesis {
    enum class LoggingLevel : uint8_t {
        GN_LOGLEVEL_TRACE2 = 1U,
        GN_LOGLEVEL_TRACE = 2U,
        GN_LOGLEVEL_DEBUG = 3U,
        GN_LOGLEVEL_INFO = 4U,
        GN_LOGLEVEL_WARNING = 5U,
        GN_LOGLEVEL_ERROR = 6U,
        GN_LOGLEVEL_CRITICAL = 7U
    };

    class Logger {
        public:
            Logger();
            ~Logger();

            static bool init(std::string applicationName);
            static void setCoreLogLevel(LoggingLevel newLoggingLevel);
            static void setClientLogLevel(LoggingLevel newLoggingLevel);

            inline static quill::Logger* getCoreLogger() { return s_coreLogger; }
            inline static quill::Logger* getClientLogger() { return s_clientLogger; }

        private:
            static quill::Logger* s_coreLogger;
            static quill::Logger* s_clientLogger;
    };
}  // namespace Genesis

#define GN_CORE_CRITICAL(...) QUILL_LOG_CRITICAL(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)
#define GN_CORE_ERROR(...) QUILL_LOG_ERROR(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)
#define GN_CORE_WARNING(...) QUILL_LOG_WARNING(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)
#define GN_CORE_INFO(...) QUILL_LOG_INFO(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)
#define GN_CORE_DEBUG(...) QUILL_LOG_DEBUG(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)
#define GN_CORE_TRACE(...) QUILL_LOG_TRACE_L1(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)
#define GN_CORE_TRACE2(...) QUILL_LOG_TRACE_L2(::Genesis::Logger::getCoreLogger(), __VA_ARGS__)

#define GN_CLIENT_CRITICAL(...) QUILL_LOG_CRITICAL(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
#define GN_CLIENT_ERROR(...) QUILL_LOG_ERROR(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
#define GN_CLIENT_WARNING(...) QUILL_LOG_WARNING(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
#define GN_CLIENT_INFO(...) QUILL_LOG_INFO(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
#define GN_CLIENT_DEBUG(...) QUILL_LOG_DEBUG(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
#define GN_CLIENT_TRACE(...) QUILL_LOG_TRACE_L1(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
#define GN_CLIENT_TRACE2(...) QUILL_LOG_TRACE_L2(::Genesis::Logger::getClientLogger(), __VA_ARGS__)
