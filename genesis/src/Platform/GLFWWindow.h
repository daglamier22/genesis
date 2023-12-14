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

            void init(const WindowCreationProperties properties);
            void shutdown();
            virtual void onUpdate();

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
