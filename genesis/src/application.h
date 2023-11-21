#pragma once

#include <string>

namespace Genesis {
    class Application {
        public:
            Application(std::string applicationName);
            virtual ~Application();

            void run();
        
        private:
            std::string m_applicationName;
    };

    Application* createApp();
}
