#include "Application.h"

#include "Core/Logger.h"

namespace Genesis {
    Application::Application(std::string applicationName) {
        m_applicationName = applicationName;
        Genesis::Logger::init(m_applicationName);
    }

    Application::~Application() {
        
    }

    void Application::run() {
        while(true) {
        }
    }
}
