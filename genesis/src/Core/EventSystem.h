#pragma once

#include "Events/Event.h"

namespace Genesis {
// Borrowed from Cherno's Hazel engine after having difficulty figuring out the function binding logic
#define GN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

    struct RegisteredListener {
            void* m_listener;
            std::function<void(Event&)> m_callback;
    };

    struct RegisteredEvent {
            std::vector<RegisteredListener*> m_registeredListeners;
    };

    class EventSystem {
        public:
            static void registerEvent(EventType type, void* listener, const std::function<void(Event&)>& callback);
            static void unregisterEvent(EventType type, void* listener, const std::function<void(Event&)>& callback);
            static void fireEvent(Event& e);

        private:
            static RegisteredEvent s_registeredEvents[(int)EventType::MAX_EVENT_TYPES];
    };
}  // namespace Genesis
