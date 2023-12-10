#include "GLFWWindow.h"

#include "Core/Logger.h"
#include "Core/EventSystem.h"
#include "Events/ApplicationEvents.h"

namespace Genesis {
    GLFWWindow::GLFWWindow(const WindowCreationProperties properties) {
        init(properties);
    }

    GLFWWindow::~GLFWWindow() {
        shutdown();
    }

    void GLFWWindow::init(const WindowCreationProperties properties) {
        m_title = properties.title;
        m_x = properties.x;
        m_y = properties.y;
        m_windowWidth = properties.width;
        m_windowHeight = properties.height;

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // TODO: Remove this later to get resizing working
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, m_title.c_str(), nullptr, nullptr);
    }

    void GLFWWindow::shutdown() {
        glfwDestroyWindow(m_window);

        glfwTerminate();
    }

    void GLFWWindow::onUpdate() {
        GN_CORE_INFO("window::onUpdate");
        while(!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
        }
        // TODO: faking app closure here
        WindowCloseEvent closeEvent;
        EventSystem::fireEvent(closeEvent);
    }
}  // namespace Genesis
