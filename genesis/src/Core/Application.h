#pragma once

#include "Window.h"

namespace Genesis {
    class Application {
        public:
            Application(std::string applicationName);
            virtual ~Application();

            void init();
            void run();

        private:
            std::string m_applicationName;
            bool m_isRunning;
            std::unique_ptr<Window> m_window;
    };

    Application* createApp();
}  // namespace Genesis
