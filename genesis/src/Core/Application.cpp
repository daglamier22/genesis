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
        using Clock = std::chrono::steady_clock;
        using duration = std::chrono::duration<double>;
        using time_point = std::chrono::time_point<Clock, duration>;
        time_point previousTime = Clock::now();
        while (m_isRunning) {
            m_window->onUpdate();
            m_renderer->drawFrame();
            time_point currentTime = Clock::now();
            auto frameTime = currentTime - previousTime;
            auto fps = 1 / frameTime.count();
            GN_CORE_TRACE2("frameTime: {}, fps: {}", frameTime, fps);

            previousTime = currentTime;
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
