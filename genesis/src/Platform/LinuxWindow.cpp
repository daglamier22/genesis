#include "Platform/LinuxWindow.h"

#include <iostream>

#include "Core/Logger.h"

namespace Genesis {
    LinuxWindow::LinuxWindow(const WindowCreationProperties properties) {
        init(properties);
    }

    LinuxWindow::~LinuxWindow() {
        shutdown();
    }

    void LinuxWindow::init(const WindowCreationProperties properties) {
        m_title = properties.title;
        m_x = properties.x;
        m_y = properties.y;
        m_windowWidth = properties.width;
        m_windowHeight = properties.height;

        // Connect to X11 server
        int screenNum;
        m_connection = xcb_connect(0, &screenNum);
        assert(!xcb_connection_has_error(m_connection));

        m_screen = xcb_aux_get_screen(m_connection, screenNum);

        // Register event types.
        // XCB_CW_BACK_PIXEL = filling then window bg with a single colour
        // XCB_CW_EVENT_MASK is required.
        uint32_t event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

        // Listen for keyboard and mouse buttons
        uint32_t event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
                                XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                                XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
                                XCB_EVENT_MASK_STRUCTURE_NOTIFY;

        xcb_create_window_value_list_t createWindowValues;
        createWindowValues.background_pixel = m_screen->black_pixel;
        createWindowValues.event_mask = event_values;

        // Create the window
        m_window = xcb_generate_id(m_connection);
        xcb_create_window_aux(m_connection,
                              XCB_COPY_FROM_PARENT,
                              m_window,
                              m_screen->root,
                              m_x,
                              m_y,
                              m_windowWidth,
                              m_windowHeight,
                              0,
                              XCB_WINDOW_CLASS_INPUT_OUTPUT,
                              m_screen->root_visual,
                              event_mask,
                              &createWindowValues);

        xcb_change_property(m_connection,
                            XCB_PROP_MODE_REPLACE,
                            m_window,
                            XCB_ATOM_WM_NAME,
                            XCB_ATOM_STRING,
                            8,
                            m_title.length(),
                            m_title.c_str());

        // Tell the server to notify when the window manager
        // attempts to destroy the window.
        xcb_intern_atom_cookie_t deleteCookie, protocolsCookie;
        xcb_intern_atom_reply_t *deleteReply, *protocolsReply;
        protocolsCookie = xcb_intern_atom(m_connection,
                                          0,
                                          strlen("WM_PROTOCOLS"),
                                          "WM_PROTOCOLS");
        deleteCookie = xcb_intern_atom(m_connection,
                                       0,
                                       strlen("WM_DELETE_WINDOW"),
                                       "WM_DELETE_WINDOW");
        protocolsReply = xcb_intern_atom_reply(m_connection,
                                               protocolsCookie,
                                               NULL);
        deleteReply = xcb_intern_atom_reply(m_connection,
                                            deleteCookie,
                                            NULL);
        m_windowManagerDeleteWindowAtom = deleteReply->atom;
        m_windowManagerProtocolsAtom = protocolsReply->atom;
        delete deleteReply;
        delete protocolsReply;

        xcb_change_property(m_connection,
                            XCB_PROP_MODE_REPLACE,
                            m_window,
                            m_windowManagerProtocolsAtom,
                            4,
                            32,
                            1,
                            &m_windowManagerDeleteWindowAtom);

        // Display the window
        xcb_map_window(m_connection, m_window);

        // Make sure commands are sent before we pause, so window is shown
        assert(xcb_flush(m_connection));
    }

    void LinuxWindow::shutdown() {
        if (m_connection) {
            xcb_disconnect(m_connection);
        }
    }

    bool LinuxWindow::onUpdate() {
        bool quit = false;
        xcb_generic_event_t* event;
        while ((event = xcb_poll_for_event(m_connection))) {
            switch (XCB_EVENT_RESPONSE_TYPE(event)) {
                case XCB_KEY_PRESS:
                case XCB_KEY_RELEASE: {
                    xcb_key_press_event_t* keyEvent = (xcb_key_press_event_t*)event;
                    bool keyPressed = event->response_type == XCB_KEY_PRESS;
                    // KeySym keySym = XkbKeycodeToKeysym();
                    xcb_key_symbols_t* symbols = xcb_key_symbols_alloc(m_connection);
                    xcb_keysym_t keySym = xcb_key_symbols_get_keysym(symbols, keyEvent->detail, 0);
                    std::ostringstream ss;
                    ss << std::hex << keySym;
                    GN_CORE_INFO("Key {s}: {d} {X}", keyPressed ? "Pressed" : "Released", keyEvent->detail, ss.str());
                    std::cout << std::hex << keySym << std::endl;
                    if (keySym == XK_Escape) {
                        quit = true;
                    }
                } break;
                case XCB_BUTTON_PRESS:
                case XCB_BUTTON_RELEASE: {
                    xcb_button_press_event_t* buttonEvent = (xcb_button_press_event_t*)event;
                    bool buttonPressed = event->response_type == XCB_BUTTON_PRESS;
                    switch (buttonEvent->detail) {
                        case XCB_BUTTON_INDEX_1: {
                            GN_CORE_INFO("Left Mouse Button {s}", buttonPressed ? "Pressed" : "Released");
                        } break;
                        case XCB_BUTTON_INDEX_2: {
                            GN_CORE_INFO("Middle Mouse Button {s}", buttonPressed ? "Pressed" : "Released");
                        } break;
                        case XCB_BUTTON_INDEX_3: {
                            GN_CORE_INFO("Right Mouse Button {s}", buttonPressed ? "Pressed" : "Released");
                        } break;
                    }
                } break;
                case XCB_MOTION_NOTIFY: {
                    xcb_motion_notify_event_t* mouseEvent = (xcb_motion_notify_event_t*)event;
                    GN_CORE_INFO("Mouse Moved: {i} {i}", mouseEvent->event_x, mouseEvent->event_y);
                } break;
                case XCB_EXPOSE: {
                    GN_CORE_INFO("Window Exposed");
                } break;
                case XCB_CLIENT_MESSAGE: {
                    xcb_client_message_event_t* clientMessage = (xcb_client_message_event_t*)event;

                    // Window close
                    if (clientMessage->data.data32[0] == m_windowManagerDeleteWindowAtom) {
                        quit = true;
                    }
                } break;
                case XCB_CONFIGURE_NOTIFY: {
                    xcb_configure_notify_event_t* configureEvent = (xcb_configure_notify_event_t*)event;
                    if (configureEvent->width != m_windowWidth || configureEvent->height != m_windowHeight) {
                        GN_CORE_INFO("Window Resized: {i} {i}", configureEvent->width, configureEvent->height);
                        m_windowWidth = configureEvent->width;
                        m_windowHeight = configureEvent->height;
                    }
                    if (configureEvent->x != m_x || configureEvent->y != m_y) {
                        GN_CORE_INFO("Window Moved: {i} {i}", configureEvent->x, configureEvent->y);
                        m_x = configureEvent->x;
                        m_y = configureEvent->y;
                    }
                } break;
                default:
                    break;
            }
        }
        return !quit;
    }
}  // namespace Genesis
