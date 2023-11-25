#include "Core/EventSystem.h"

#include "Core/Logger.h"
#include "Events/Event.h"

namespace Genesis {
    RegisteredEvent EventSystem::s_registeredEvents[(unsigned long)EventType::MAX_EVENT_TYPES];

    void EventSystem::registerEvent(EventType type, void* listener, const std::function<void(Event&)>& callback) {
        for (auto it = s_registeredEvents[(int)type].m_registeredListeners.begin(); it != s_registeredEvents[(int)type].m_registeredListeners.end(); ++it) {
            if ((*it)->m_listener == listener) {
                GN_CORE_WARNING("Event ({d}) already registered for this listener {s}", (int)type);
                return;
            }
        }

        RegisteredListener* newListener = new RegisteredListener();
        newListener->m_listener = listener;
        newListener->m_callback = callback;
        s_registeredEvents[(int)type].m_registeredListeners.push_back(newListener);
    }

    void EventSystem::unregisterEvent(EventType type, void* listener, const std::function<void(Event&)>& callback) {
        for (auto it = s_registeredEvents[(int)type].m_registeredListeners.begin(); it != s_registeredEvents[(int)type].m_registeredListeners.end(); ++it) {
            if ((*it)->m_listener == listener) {
                s_registeredEvents[(int)type].m_registeredListeners.erase(it);
                delete (*it);
                return;
            }
        }
        GN_CORE_WARNING("Event ({d}) was not registered before attempting to unregister for this listener", (int)type);
    }

    void EventSystem::fireEvent(Event& e) {
        for (auto it = s_registeredEvents[(int)e.getEventType()].m_registeredListeners.begin(); it != s_registeredEvents[(int)e.getEventType()].m_registeredListeners.end(); ++it) {
            GN_CORE_TRACE("Event fired: {s}", e.toString());
            (*it)->m_callback(e);
        }
    }
}  // namespace Genesis
