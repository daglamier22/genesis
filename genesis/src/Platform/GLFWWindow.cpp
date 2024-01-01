#include "GLFWWindow.h"

#include "Core/EventSystem.h"
#include "Core/Logger.h"
#include "Events/ApplicationEvents.h"
#include "Events/KeyboardEvents.h"
#include "Events/MouseEvents.h"

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

        m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, m_title.c_str(), nullptr, nullptr);

        glfwSetWindowUserPointer(m_window, this);

        glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
            GLFWWindow& data = *(GLFWWindow*)glfwGetWindowUserPointer(window);
            data.m_windowWidth = width;
            data.m_windowHeight = height;

            GN_CORE_TRACE("Window resize: {} {}", width, height);
            WindowResizeEvent resizeEvent(width, height);
            EventSystem::fireEvent(resizeEvent);
        });

        glfwSetWindowPosCallback(m_window, [](GLFWwindow* window, int xpos, int ypos) {
            GLFWWindow& data = *(GLFWWindow*)glfwGetWindowUserPointer(window);
            data.m_x = xpos;
            data.m_y = ypos;

            GN_CORE_TRACE("Window move occured. {}, {}", xpos, ypos);
        });

        glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
            GLFWWindow& data = *(GLFWWindow*)glfwGetWindowUserPointer(window);
            GN_CORE_TRACE("Window close event triggered.");
            WindowCloseEvent closeEvent;
            EventSystem::fireEvent(closeEvent);
        });

        glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            GLFWWindow& data = *(GLFWWindow*)glfwGetWindowUserPointer(window);

            switch (action) {
                case GLFW_PRESS: {
                    bool keyPressed = true;
                    GN_CORE_TRACE("Key pressed: {} {} {} {}", key, scancode, action, mods);
                    KeyEvent keyboardEvent(data.translateKey(key), keyPressed);
                    EventSystem::fireEvent(keyboardEvent);
                    break;
                }
                case GLFW_RELEASE: {
                    bool keyPressed = false;
                    GN_CORE_TRACE("Key released: {} {} {} {}", key, scancode, action, mods);
                    KeyEvent keyboardEvent(data.translateKey(key), keyPressed);
                    EventSystem::fireEvent(keyboardEvent);
                    break;
                }
                case GLFW_REPEAT: {
                    bool keyPressed = true;
                    GN_CORE_TRACE("Key repeated: {} {} {} {}", key, scancode, action, mods);
                    KeyEvent keyboardEvent(data.translateKey(key), keyPressed);
                    EventSystem::fireEvent(keyboardEvent);
                    break;
                }
            }
        });

        // glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int keycode) {
        //     GLFWWindow& data = *(GLFWWindow*)glfwGetWindowUserPointer(window);

        //     bool keyPressed = true;
        //     KeyEvent keyboardEvent(translateKey(keySym), keyPressed);
        //     EventSystem::fireEvent(keyboardEvent);
        // });

        glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
            GLFWWindow& data = *(GLFWWindow*)glfwGetWindowUserPointer(window);

            switch (action) {
                case GLFW_PRESS: {
                    bool buttonPressed = true;
                    GN_CORE_TRACE("Mouse button pressed: {} {}", button, action);
                    MouseButtonEvent mouseEvent(data.translateButton(button), buttonPressed);
                    EventSystem::fireEvent(mouseEvent);
                    break;
                }
                case GLFW_RELEASE: {
                    bool buttonPressed = false;
                    GN_CORE_TRACE("Mouse button released: {} {}", button, action);
                    MouseButtonEvent mouseEvent(data.translateButton(button), buttonPressed);
                    EventSystem::fireEvent(mouseEvent);
                    break;
                }
            }
        });

        glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset) {
            GLFWWindow& data = *(GLFWWindow*)glfwGetWindowUserPointer(window);

            GN_CORE_TRACE("Mouse scroll event: {} {}", xOffset, yOffset);
            MouseScrollEvent scrollEvent((int16_t)yOffset);
            EventSystem::fireEvent(scrollEvent);
        });

        glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos) {
            GLFWWindow& data = *(GLFWWindow*)glfwGetWindowUserPointer(window);

            GN_CORE_TRACE("Mouse Moved: {} {}", xPos, yPos);
            MouseMoveEvent moveEvent((int16_t)xPos, (int16_t)yPos);
            EventSystem::fireEvent(moveEvent);
        });
    }

    void GLFWWindow::shutdown() {
        glfwDestroyWindow(m_window);

        glfwTerminate();
    }

    void GLFWWindow::onUpdate() {
        glfwPollEvents();
    }

    void GLFWWindow::waitForWindowToBeRestored(int* width, int* height) {
        glfwGetFramebufferSize(m_window, width, height);
        glfwWaitEvents();
    }

    bool GLFWWindow::createVulkanSurface(VkInstance instance, VkSurfaceKHR* surface) {
        if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS) {
            return false;
        }

        return true;
    }

    const char** GLFWWindow::getRequiredVulkanInstanceExtensions(uint32_t* glfwExtensionCount) {
        return glfwGetRequiredInstanceExtensions(glfwExtensionCount);
    }

    void GLFWWindow::updateTitle(double currentFps) {
        std::stringstream title;
        title << m_title << " running at " << currentFps << " fps.";
        glfwSetWindowTitle(m_window, title.str().c_str());
    }

    Key GLFWWindow::translateKey(int key) {
        switch (key) {
            case GLFW_KEY_BACKSPACE:
                return Key::KEY_BACKSPACE;
            case GLFW_KEY_ENTER:
            case GLFW_KEY_KP_ENTER:
                return Key::KEY_ENTER;
            case GLFW_KEY_TAB:
                return Key::KEY_TAB;

            case GLFW_KEY_PAUSE:
                return Key::KEY_PAUSE;
            case GLFW_KEY_CAPS_LOCK:
                return Key::KEY_CAPITAL;

            case GLFW_KEY_ESCAPE:
                return Key::KEY_ESCAPE;

            case GLFW_KEY_SPACE:
                return Key::KEY_SPACE;
            case GLFW_KEY_PAGE_UP:
                return Key::KEY_PAGEUP;
            case GLFW_KEY_PAGE_DOWN:
                return Key::KEY_PAGEDOWN;
            case GLFW_KEY_END:
                return Key::KEY_END;
            case GLFW_KEY_HOME:
                return Key::KEY_HOME;
            case GLFW_KEY_LEFT:
                return Key::KEY_LEFT;
            case GLFW_KEY_UP:
                return Key::KEY_UP;
            case GLFW_KEY_RIGHT:
                return Key::KEY_RIGHT;
            case GLFW_KEY_DOWN:
                return Key::KEY_DOWN;
            case GLFW_KEY_PRINT_SCREEN:
                return Key::KEY_PRINT;
            case GLFW_KEY_INSERT:
                return Key::KEY_INSERT;
            case GLFW_KEY_DELETE:
                return Key::KEY_DELETE;

            case GLFW_KEY_LEFT_SUPER:
                return Key::KEY_LSUPER;
            case GLFW_KEY_RIGHT_SUPER:
                return Key::KEY_RSUPER;

            case GLFW_KEY_KP_0:
                return Key::KEY_NUMPAD0;
            case GLFW_KEY_KP_1:
                return Key::KEY_NUMPAD1;
            case GLFW_KEY_KP_2:
                return Key::KEY_NUMPAD2;
            case GLFW_KEY_KP_3:
                return Key::KEY_NUMPAD3;
            case GLFW_KEY_KP_4:
                return Key::KEY_NUMPAD4;
            case GLFW_KEY_KP_5:
                return Key::KEY_NUMPAD5;
            case GLFW_KEY_KP_6:
                return Key::KEY_NUMPAD6;
            case GLFW_KEY_KP_7:
                return Key::KEY_NUMPAD7;
            case GLFW_KEY_KP_8:
                return Key::KEY_NUMPAD8;
            case GLFW_KEY_KP_9:
                return Key::KEY_NUMPAD9;
            case GLFW_KEY_KP_MULTIPLY:
                return Key::KEY_MULTIPLY;
            case GLFW_KEY_KP_ADD:
                return Key::KEY_ADD;
            case GLFW_KEY_KP_SUBTRACT:
                return Key::KEY_SUBTRACT;
            case GLFW_KEY_KP_DECIMAL:
                return Key::KEY_DECIMAL;
            case GLFW_KEY_KP_DIVIDE:
                return Key::KEY_DIVIDE;
            case GLFW_KEY_F1:
                return Key::KEY_F1;
            case GLFW_KEY_F2:
                return Key::KEY_F2;
            case GLFW_KEY_F3:
                return Key::KEY_F3;
            case GLFW_KEY_F4:
                return Key::KEY_F4;
            case GLFW_KEY_F5:
                return Key::KEY_F5;
            case GLFW_KEY_F6:
                return Key::KEY_F6;
            case GLFW_KEY_F7:
                return Key::KEY_F7;
            case GLFW_KEY_F8:
                return Key::KEY_F8;
            case GLFW_KEY_F9:
                return Key::KEY_F9;
            case GLFW_KEY_F10:
                return Key::KEY_F10;
            case GLFW_KEY_F11:
                return Key::KEY_F11;
            case GLFW_KEY_F12:
                return Key::KEY_F12;
            case GLFW_KEY_F13:
                return Key::KEY_F13;
            case GLFW_KEY_F14:
                return Key::KEY_F14;
            case GLFW_KEY_F15:
                return Key::KEY_F15;
            case GLFW_KEY_F16:
                return Key::KEY_F16;
            case GLFW_KEY_F17:
                return Key::KEY_F17;
            case GLFW_KEY_F18:
                return Key::KEY_F18;
            case GLFW_KEY_F19:
                return Key::KEY_F19;
            case GLFW_KEY_F20:
                return Key::KEY_F20;
            case GLFW_KEY_F21:
                return Key::KEY_F21;
            case GLFW_KEY_F22:
                return Key::KEY_F22;
            case GLFW_KEY_F23:
                return Key::KEY_F23;
            case GLFW_KEY_F24:
                return Key::KEY_F24;

            case GLFW_KEY_NUM_LOCK:
                return Key::KEY_NUMLOCK;
            case GLFW_KEY_SCROLL_LOCK:
                return Key::KEY_SCROLL;

            case GLFW_KEY_KP_EQUAL:
                return Key::KEY_NUMPAD_EQUAL;

            case GLFW_KEY_LEFT_SHIFT:
                return Key::KEY_LSHIFT;
            case GLFW_KEY_RIGHT_SHIFT:
                return Key::KEY_RSHIFT;
            case GLFW_KEY_LEFT_CONTROL:
                return Key::KEY_LCONTROL;
            case GLFW_KEY_RIGHT_CONTROL:
                return Key::KEY_RCONTROL;
            case GLFW_KEY_LEFT_ALT:
                return Key::KEY_LALT;
            case GLFW_KEY_RIGHT_ALT:
                return Key::KEY_RALT;

            case GLFW_KEY_SEMICOLON:
                return Key::KEY_SEMICOLON;
            case GLFW_KEY_APOSTROPHE:
                return Key::KEY_APOSTROPHE;
            case GLFW_KEY_EQUAL:
                return Key::KEY_EQUAL;
            case GLFW_KEY_COMMA:
                return Key::KEY_COMMA;
            case GLFW_KEY_MINUS:
                return Key::KEY_MINUS;
            case GLFW_KEY_PERIOD:
                return Key::KEY_PERIOD;
            case GLFW_KEY_SLASH:
                return Key::KEY_SLASH;
            case GLFW_KEY_GRAVE_ACCENT:
                return Key::KEY_GRAVE;

            case GLFW_KEY_0:
                return Key::KEY_0;
            case GLFW_KEY_1:
                return Key::KEY_1;
            case GLFW_KEY_2:
                return Key::KEY_2;
            case GLFW_KEY_3:
                return Key::KEY_3;
            case GLFW_KEY_4:
                return Key::KEY_4;
            case GLFW_KEY_5:
                return Key::KEY_5;
            case GLFW_KEY_6:
                return Key::KEY_6;
            case GLFW_KEY_7:
                return Key::KEY_7;
            case GLFW_KEY_8:
                return Key::KEY_8;
            case GLFW_KEY_9:
                return Key::KEY_9;

            case GLFW_KEY_A:
                return Key::KEY_A;
            case GLFW_KEY_B:
                return Key::KEY_B;
            case GLFW_KEY_C:
                return Key::KEY_C;
            case GLFW_KEY_D:
                return Key::KEY_D;
            case GLFW_KEY_E:
                return Key::KEY_E;
            case GLFW_KEY_F:
                return Key::KEY_F;
            case GLFW_KEY_G:
                return Key::KEY_G;
            case GLFW_KEY_H:
                return Key::KEY_H;
            case GLFW_KEY_I:
                return Key::KEY_I;
            case GLFW_KEY_J:
                return Key::KEY_J;
            case GLFW_KEY_K:
                return Key::KEY_K;
            case GLFW_KEY_L:
                return Key::KEY_L;
            case GLFW_KEY_M:
                return Key::KEY_M;
            case GLFW_KEY_N:
                return Key::KEY_N;
            case GLFW_KEY_O:
                return Key::KEY_O;
            case GLFW_KEY_P:
                return Key::KEY_P;
            case GLFW_KEY_Q:
                return Key::KEY_Q;
            case GLFW_KEY_R:
                return Key::KEY_R;
            case GLFW_KEY_S:
                return Key::KEY_S;
            case GLFW_KEY_T:
                return Key::KEY_T;
            case GLFW_KEY_U:
                return Key::KEY_U;
            case GLFW_KEY_V:
                return Key::KEY_V;
            case GLFW_KEY_W:
                return Key::KEY_W;
            case GLFW_KEY_X:
                return Key::KEY_X;
            case GLFW_KEY_Y:
                return Key::KEY_Y;
            case GLFW_KEY_Z:
                return Key::KEY_Z;

            case GLFW_KEY_LEFT_BRACKET:
                return Key::KEY_LBRACKET;
            case GLFW_KEY_BACKSLASH:
                return Key::KEY_BACKSLASH;
            case GLFW_KEY_RIGHT_BRACKET:
                return Key::KEY_RBRACKET;

            case GLFW_KEY_MENU:
                return Key::KEY_MENU;

            default:
                return Key::KEYS_MAX_KEYS;
        }
    }

    Button GLFWWindow::translateButton(int button) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                return Button::BUTTON_LEFT;
            case GLFW_MOUSE_BUTTON_RIGHT:
                return Button::BUTTON_RIGHT;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                return Button::BUTTON_MIDDLE;

            default:
                return Button::BUTTON_MAX_BUTTONS;
        }
    }
}  // namespace Genesis
