#pragma once

#include "Event.h"

namespace Genesis {
    class KeyEvent : public Event {
        public:
            KeyEvent(const int key, const bool pressed) : m_key(key), m_pressed(pressed) {
            }

            virtual EventType getEventType() const override { return EventType::MouseMove; }
            virtual std::string toString() const override {
                std::stringstream ss;
                ss << "Key: " << m_key << " " << m_pressed ? "pressed" : "released";
                return ss.str();
            }

        private:
            int m_key;
            bool m_pressed;
    };
}  // namespace Genesis
