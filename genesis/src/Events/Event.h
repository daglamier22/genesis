#pragma once

namespace Genesis {
    enum class EventType {
        None = 0,
        WindowClose,
        WindowResize,
        MouseMove,
        // This should always be last as a way to get the total number of event types
        MAX_EVENT_TYPES
    };
    class Event {
        public:
            virtual ~Event() = default;
            virtual EventType getEventType() const = 0;
            virtual std::string toString() const = 0;

            bool handled = false;
    };
}  // namespace Genesis
