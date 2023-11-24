#include "Application.h"

#include "Logger.h"

namespace Genesis {
    Application::Application(std::string applicationName) {
        m_applicationName = applicationName;
        init();
    }

    Application::~Application() {
    }

    void Application::init() {
        Genesis::Logger::init(m_applicationName);
        WindowCreationProperties windowProperties;
        m_window = Window::create(windowProperties);
        m_isRunning = true;
    }

    void Application::run() {
        while (m_isRunning) {
            m_isRunning = m_window->onUpdate();
        }
    }
}  // namespace Genesis
