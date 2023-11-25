#pragma once

#include "Event.h"

namespace Genesis {
    class MouseMoveEvent : public Event {
        public:
            MouseMoveEvent(const float x, const float y) : m_mouseX(x), m_mouseY(y) {
            }

            virtual EventType getEventType() const override { return EventType::MouseMove; }
            virtual std::string toString() const override {
                std::stringstream ss;
                ss << "MouseMove: " << m_mouseX << ", " << m_mouseY;
                return ss.str();
            }

        private:
            float m_mouseX;
            float m_mouseY;
    };

    class MouseScrollEvent : public Event {
        public:
            MouseScrollEvent(const float xOffset, const float yOffset) : m_xOffset(xOffset), m_yOffset(yOffset) {
            }

            virtual EventType getEventType() const override { return EventType::MouseScroll; }
            virtual std::string toString() const override {
                std::stringstream ss;
                ss << "MouseScroll: " << m_xOffset << ", " << m_yOffset;
                return ss.str();
            }

        private:
            float m_xOffset;
            float m_yOffset;
    };

    class MouseButtonEvent : public Event {
        public:
            MouseButtonEvent(const int button, const bool pressed) : m_button(button), m_pressed(pressed) {
            }

            virtual EventType getEventType() const override { return EventType::MouseButton; }
            virtual std::string toString() const override {
                std::stringstream ss;
                ss << "MouseButton: " << m_button << " " << m_pressed ? "pressed" : "released";
                return ss.str();
            }

        private:
            int m_button;
            bool m_pressed;
    };
}  // namespace Genesis
