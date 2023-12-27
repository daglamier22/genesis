#pragma once

#include "../Events/ApplicationEvents.h"
#include "InputSystem.h"
#include "Renderer.h"
#include "Window.h"

namespace Genesis {
    class Application {
        public:
            Application(std::string applicationName);
            virtual ~Application();

            void init();
            void run();
            void shutdown();

            void onCloseEvent(Event& e);

        private:
            void calculateFrameRate();

            std::string m_applicationName;

            bool m_isRunning;
            using Clock = std::chrono::steady_clock;
            using duration = std::chrono::duration<double>;
            using time_point = std::chrono::time_point<Clock, duration>;
            time_point m_previousTime;
            time_point m_currentTime;

            std::shared_ptr<Window> m_window;
            std::unique_ptr<InputSystem> m_inputSystem;
            std::unique_ptr<Renderer> m_renderer;
    };

    Application* createApp();
}  // namespace Genesis
