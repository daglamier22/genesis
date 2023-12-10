#pragma once

#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_util.h>

#include "Core/InputSystem.h"
#include "Core/Window.h"

namespace Genesis {
    class LinuxWindow : public Window {
        public:
            LinuxWindow(const WindowCreationProperties properties);
            virtual ~LinuxWindow();

            void init(const WindowCreationProperties properties);
            void shutdown();
            virtual void onUpdate();

        private:
            Key translateKey(xcb_keysym_t key);
            Button translateButton(xcb_button_t button);
            int16_t translateScroll(xcb_button_t button);

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
