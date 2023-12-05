#pragma once

#include "../Events/ApplicationEvents.h"
#include "InputSystem.h"
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
            void onResizeEvent(Event& e);

        private:
            std::string m_applicationName;
            bool m_isRunning;
            std::unique_ptr<Window> m_window;
            std::unique_ptr<InputSystem> m_inputSystem;
    };

    Application* createApp();
}  // namespace Genesis
