#pragma once

#include "Event.h"

namespace Genesis {
    class WindowCloseEvent : public Event {
        public:
            WindowCloseEvent() = default;

            virtual EventType getEventType() const override { return EventType::WindowClose; }
            virtual std::string toString() const override { return "WindowClose"; }
    };

    class WindowResizeEvent : public Event {
        public:
            WindowResizeEvent(unsigned int width, unsigned int height) : m_width(width), m_height(height) {
            }

            virtual EventType getEventType() const override { return EventType::WindowResize; }
            virtual std::string toString() const override {
                std::stringstream ss;
                ss << "WindowResize: " << m_width << ", " << m_height;
                return ss.str();
            }

            unsigned int getWidth() const { return m_width; }
            unsigned int getHeight() const { return m_height; }

        private:
            unsigned int m_width;
            unsigned int m_height;
    };
}  // namespace Genesis
