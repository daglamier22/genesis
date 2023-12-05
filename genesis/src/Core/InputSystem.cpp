#include "InputSystem.h"

#include "EventSystem.h"
#include "Events/ApplicationEvents.h"

namespace Genesis {
    bool InputSystem::m_keyStatuses[(int)Key::KEYS_MAX_KEYS];
    bool InputSystem::m_mouseStatuses[(int)Button::BUTTON_MAX_BUTTONS];
    int16_t InputSystem::m_mouseX;
    int16_t InputSystem::m_mouseY;

    InputSystem::InputSystem() {
        init();
    }

    InputSystem::~InputSystem() {
        shutdown();
    }

    void InputSystem::init() {
        for (int i = 0; i < (int)Key::KEYS_MAX_KEYS; ++i) {
            m_keyStatuses[i] = false;
        }
        EventSystem::registerEvent(EventType::Key, 0, GN_BIND_EVENT_FN_STATIC(InputSystem::onKeyEvent));
        EventSystem::registerEvent(EventType::MouseButton, 0, GN_BIND_EVENT_FN_STATIC(InputSystem::onMouseButtonEvent));
        EventSystem::registerEvent(EventType::MouseMove, 0, GN_BIND_EVENT_FN_STATIC(InputSystem::onMouseMoveEvent));
        EventSystem::registerEvent(EventType::MouseScroll, 0, GN_BIND_EVENT_FN_STATIC(InputSystem::onMouseScrollEvent));
    }

    void InputSystem::shutdown() {
        EventSystem::unregisterEvent(EventType::Key, 0, GN_BIND_EVENT_FN_STATIC(InputSystem::onKeyEvent));
        EventSystem::unregisterEvent(EventType::MouseButton, 0, GN_BIND_EVENT_FN_STATIC(InputSystem::onMouseButtonEvent));
        EventSystem::unregisterEvent(EventType::MouseMove, 0, GN_BIND_EVENT_FN_STATIC(InputSystem::onMouseMoveEvent));
        EventSystem::unregisterEvent(EventType::MouseScroll, 0, GN_BIND_EVENT_FN_STATIC(InputSystem::onMouseScrollEvent));
    }

    void InputSystem::onKeyEvent(const Event& e) {
        const KeyEvent keyEvent = static_cast<const KeyEvent&>(e);

        m_keyStatuses[(int)keyEvent.getKey()] = keyEvent.getPressed();

        // TODO: This should be part of a keybind system that gets called here for every key press in the future
        if (keyEvent.getKey() == Key::KEY_ESCAPE) {
            // if escape key is pressed, send close event
            WindowCloseEvent closeEvent;
            EventSystem::fireEvent(closeEvent);
        }
    }

    void InputSystem::onMouseButtonEvent(const Event& e) {
        const MouseButtonEvent mouseEvent = static_cast<const MouseButtonEvent&>(e);
        m_mouseStatuses[(int)mouseEvent.getButton()] = mouseEvent.getPressed();
    }

    void InputSystem::onMouseMoveEvent(const Event& e) {
        const MouseMoveEvent mouseEvent = static_cast<const MouseMoveEvent&>(e);
        m_mouseX = mouseEvent.getX();
        m_mouseY = mouseEvent.getY();
    }

    void InputSystem::onMouseScrollEvent(const Event& e) {
        const MouseScrollEvent scrollEvent = static_cast<const MouseScrollEvent&>(e);
    }
}  // namespace Genesis
