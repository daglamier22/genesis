#pragma once

#include "../Core/Mouse.h"
#include "Event.h"

namespace Genesis {
    class MouseMoveEvent : public Event {
        public:
            MouseMoveEvent(const int16_t x, const int16_t y) : m_mouseX(x), m_mouseY(y) {
            }

            virtual EventType getEventType() const override { return EventType::MouseMove; }
            virtual std::string toString() const override {
                std::stringstream ss;
                ss << "MouseMove: " << m_mouseX << ", " << m_mouseY;
                return ss.str();
            }
            int16_t getX() const { return m_mouseX; }
            int16_t getY() const { return m_mouseY; }

        private:
            int16_t m_mouseX;
            int16_t m_mouseY;
    };

    class MouseScrollEvent : public Event {
        public:
            MouseScrollEvent(const int16_t zDelta) : m_zDelta(zDelta) {
            }

            virtual EventType getEventType() const override { return EventType::MouseScroll; }
            virtual std::string toString() const override {
                std::stringstream ss;
                ss << "MouseScroll: " << m_zDelta;
                return ss.str();
            }
            int16_t getDelta() const { return m_zDelta; }

        private:
            int16_t m_zDelta;
    };

    class MouseButtonEvent : public Event {
        public:
            MouseButtonEvent(const Button button, const bool pressed) : m_button(button), m_pressed(pressed) {
            }

            virtual EventType getEventType() const override { return EventType::MouseButton; }
            virtual std::string toString() const override {
                std::stringstream ss;
                ss << "MouseButton: " << BUTTONTOSTR(m_button) << " " << (m_pressed ? "pressed" : "released");
                return ss.str();
            }
            Button getButton() const { return m_button; }
            bool getPressed() const { return m_pressed; }

        private:
            Button m_button;
            bool m_pressed;
    };
}  // namespace Genesis
