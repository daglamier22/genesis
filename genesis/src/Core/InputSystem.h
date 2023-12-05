#pragma once

#include "../Events/KeyboardEvents.h"
#include "../Events/MouseEvents.h"
#include "Keyboard.h"
#include "Mouse.h"

namespace Genesis {
    class InputSystem {
        public:
            InputSystem();
            ~InputSystem();

            static void init();
            static void shutdown();

            static void onKeyEvent(const Event& e);
            static void onMouseButtonEvent(const Event& e);
            static void onMouseMoveEvent(const Event& e);
            static void onMouseScrollEvent(const Event& e);

        private:
            static bool m_keyStatuses[(int)Key::KEYS_MAX_KEYS];
            static bool m_mouseStatuses[(int)Button::BUTTON_MAX_BUTTONS];
            static int16_t m_mouseX;
            static int16_t m_mouseY;
    };
}  // namespace Genesis
