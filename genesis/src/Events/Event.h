#pragma once

namespace Genesis {
    enum class EventType {
        None = 0,
        WindowClose,
        WindowResize,
        MouseMove,
        MouseScroll,
        MouseButton,
        Key,
        // This should always be last as a way to get the total number of event types
        MAX_EVENT_TYPES
    };

    const std::string eventTypeStrings[(int)EventType::MAX_EVENT_TYPES] = {
        "None",
        "WindowClose",
        "WindowResize",
        "MouseMove",
        "MouseScroll",
        "MouseButton",
        "Key"};

    class Event {
        public:
            virtual ~Event() = default;
            virtual EventType getEventType() const = 0;
            virtual std::string toString() const = 0;

            bool handled = false;
    };
}  // namespace Genesis

#define GETEVENTTYPESTR(e) (::Genesis::eventTypeStrings[(int)e])
