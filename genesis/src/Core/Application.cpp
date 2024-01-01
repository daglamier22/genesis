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
        m_scene = std::make_shared<Scene>(Scene());
        m_isRunning = true;
    }

    void Application::run() {
        m_previousTime = Clock::now();

        while (m_isRunning) {
            m_window->onUpdate();
            std::shared_ptr<GLFWWindow> window = std::dynamic_pointer_cast<GLFWWindow>(m_window);
            window->updateTitle(m_currentFps);
            m_renderer->drawFrame(m_scene);
            calculateFrameRate();
        }

        m_renderer->waitForIdle();
    }

    void Application::shutdown() {
        EventSystem::unregisterEvent(EventType::WindowClose, this, GN_BIND_EVENT_FN(Application::onCloseEvent));
    }

    void Application::onCloseEvent(Event& e) {
        m_isRunning = false;
    }

    void Application::calculateFrameRate() {
        using Clock = std::chrono::steady_clock;
        using duration = std::chrono::duration<double>;
        using time_point = std::chrono::time_point<Clock, duration>;
        m_currentTime = Clock::now();
        auto frameTime = m_currentTime - m_previousTime;
        m_currentFps = 1 / frameTime.count();
        GN_CORE_TRACE2("frameTime: {}, fps: {}", frameTime, m_currentFps);

        m_previousTime = m_currentTime;
    }
}  // namespace Genesis
