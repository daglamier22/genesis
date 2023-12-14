#include "Application.h"

#include "../Renderer/Vulkan/VulkanRenderer.h"
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
        m_inputSystem = std::make_unique<InputSystem>();
        m_renderer = std::make_unique<VulkanRenderer>(m_window);
        m_isRunning = true;
    }

    void Application::run() {
        while (m_isRunning) {
            GN_CORE_INFO("application::run");
            m_window->onUpdate();
            m_renderer->drawFrame();
        }

        m_renderer->waitForIdle();
    }

    void Application::shutdown() {
        EventSystem::unregisterEvent(EventType::WindowClose, this, GN_BIND_EVENT_FN(Application::onCloseEvent));
        EventSystem::unregisterEvent(EventType::WindowResize, this, GN_BIND_EVENT_FN(Application::onResizeEvent));
    }

    void Application::onCloseEvent(Event& e) {
        m_isRunning = false;
    }

    void Application::onResizeEvent(Event& e) {
        GN_CORE_TRACE("Resized");
    }
}  // namespace Genesis
