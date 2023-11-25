#pragma once

// #include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_util.h>
// #include <xcb/xkb.h>

#include "Core/Window.h"

namespace Genesis {
    class LinuxWindow : public Window {
        public:
            LinuxWindow(const WindowCreationProperties properties);
            ~LinuxWindow();

            void init(const WindowCreationProperties properties);
            void shutdown();
            virtual void onUpdate();

        private:
            std::string m_title;
            int16_t m_x;
            int16_t m_y;
            uint16_t m_windowWidth;
            uint16_t m_windowHeight;

            xcb_connection_t* m_connection;
            xcb_screen_t* m_screen;
            xcb_window_t m_window;
            xcb_atom_t m_windowManagerDeleteWindowAtom;
            xcb_atom_t m_windowManagerProtocolsAtom;
    };
}  // namespace Genesis
