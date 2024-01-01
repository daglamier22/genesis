#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Core/Keyboard.h"
#include "Core/Mouse.h"
#include "Core/Window.h"

namespace Genesis {
    class GLFWWindow : public Window {
        public:
            GLFWWindow(const WindowCreationProperties properties);
            virtual ~GLFWWindow();

            virtual void* getWindow() const { return m_window; }
            std::string getWindowTitle() const { return m_title; }
            uint16_t getWindowWidth() const { return m_windowWidth; }
            uint16_t getWindowHeight() const { return m_windowHeight; }

            void init(const WindowCreationProperties properties);
            void shutdown();
            virtual void onUpdate();

            void waitForWindowToBeRestored(int* width, int* height);

            bool createVulkanSurface(VkInstance instance, VkSurfaceKHR* surface);
            const char** getRequiredVulkanInstanceExtensions(uint32_t* glfwExtensionCount);
            void updateTitle(double currentFps);

        private:
            Key translateKey(int key);
            Button translateButton(int button);

            std::string m_title;
            int16_t m_x;
            int16_t m_y;
            uint16_t m_windowWidth;
            uint16_t m_windowHeight;

            GLFWwindow* m_window;
    };
}  // namespace Genesis
