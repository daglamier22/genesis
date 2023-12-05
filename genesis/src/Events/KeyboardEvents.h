#pragma once

#include "../Core/Keyboard.h"
#include "Event.h"

namespace Genesis {
    class KeyEvent : public Event {
        public:
            KeyEvent(const Key key, const bool pressed) : m_key(key), m_pressed(pressed) {
            }

            virtual EventType getEventType() const override { return EventType::Key; }
            virtual std::string toString() const override {
                std::stringstream ss;
                ss << "Key: " << KEYTOSTR(m_key) << " " << (m_pressed ? "pressed" : "released");
                return ss.str();
            }
            Key getKey() const { return m_key; }
            bool getPressed() const { return m_pressed; }

        private:
            Key m_key;
            bool m_pressed;
    };
}  // namespace Genesis
