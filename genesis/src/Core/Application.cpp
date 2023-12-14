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
        windowProperties.title = m_applicationName;
        m_window = Window::create(windowProperties);
        EventSystem::registerEvent(EventType::WindowClose, this, GN_BIND_EVENT_FN(Application::onCloseEvent));
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
    }

    void Application::onCloseEvent(Event& e) {
        m_isRunning = false;
    }
}  // namespace Genesis
