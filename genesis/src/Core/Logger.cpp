#include "Logger.h"

namespace Genesis {
    std::shared_ptr<quill::Logger> Logger::s_coreLogger;
    std::shared_ptr<quill::Logger> Logger::s_clientLogger;

    Logger::Logger() {
    }

    Logger::~Logger() {
    }

    bool Logger::init(std::string applicationName) {
        quill::Config cfg;
        cfg.enable_console_colours = true;
        quill::configure(cfg);

        quill::start();

        std::shared_ptr<quill::Handler> handler = quill::stdout_handler();
        handler->set_pattern("%(ascii_time) [%(thread)] %(logger_name:<10) %(fileline:<28) LOG_%(level_name) %(message)");
        s_coreLogger = std::shared_ptr<quill::Logger>(quill::create_logger("Genesis", std::move(handler)));

        std::shared_ptr<quill::Handler> handler2 = quill::stdout_handler();
        handler2->set_pattern("%(ascii_time) [%(thread)] %(logger_name:<10) %(fileline:<28) LOG_%(level_name) %(message)");
        s_clientLogger = std::shared_ptr<quill::Logger>(quill::create_logger(applicationName, std::move(handler2)));

        return true;
    }

    void Logger::setCoreLogLevel(LoggingLevel newLoggingLevel) {
        if (s_coreLogger) {
            s_coreLogger->set_log_level((quill::LogLevel)newLoggingLevel);
        }
    }

    void Logger::setClientLogLevel(LoggingLevel newLoggingLevel) {
        if (s_clientLogger) {
            s_clientLogger->set_log_level((quill::LogLevel)newLoggingLevel);
        }
    }
}  // namespace Genesis
