#include "Application.h"

#include "Core/EventSystem.h"
#include "Core/Logger.h"

namespace Genesis {
    Application::Application(std::string applicationName) : m_applicationName(applicationName) {
        init();
    }

    Application::~Application() {
        shutdown();
    }

    void Application::init() {
        Genesis::Logger::init(m_applicationName);
        WindowCreationProperties windowProperties;
        m_window = Window::create(windowProperties);
        EventSystem::registerEvent(EventType::WindowClose, this, GN_BIND_EVENT_FN(Application::onCloseEvent));
        EventSystem::registerEvent(EventType::WindowResize, this, GN_BIND_EVENT_FN(Application::onResizeEvent));
        m_isRunning = true;
    }

    void Application::run() {
        while (m_isRunning) {
            m_window->onUpdate();
        }
    }

    void Application::shutdown() {
        EventSystem::unregisterEvent(EventType::WindowClose, this, GN_BIND_EVENT_FN(Application::onCloseEvent));
        EventSystem::unregisterEvent(EventType::WindowResize, this, GN_BIND_EVENT_FN(Application::onResizeEvent));
    }

    void Application::onCloseEvent(Event& e) {
        m_isRunning = false;
    }

    void Application::onResizeEvent(Event& e) {
        GN_CORE_INFO("Resized");
    }
}  // namespace Genesis
