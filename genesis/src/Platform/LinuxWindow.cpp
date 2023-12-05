#include "Platform/LinuxWindow.h"

#include "Core/EventSystem.h"
#include "Core/Logger.h"
#include "Events/ApplicationEvents.h"
#include "Events/KeyboardEvents.h"
#include "Events/MouseEvents.h"

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

    void LinuxWindow::onUpdate() {
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
                    GN_CORE_TRACE("Key {s}: {d} {X}", keyPressed ? "Pressed" : "Released", keyEvent->detail, ss.str());

                    KeyEvent keyboardEvent(translateKey(keySym), keyPressed);
                    EventSystem::fireEvent(keyboardEvent);
                } break;
                case XCB_BUTTON_PRESS:
                case XCB_BUTTON_RELEASE: {
                    xcb_button_press_event_t* buttonEvent = (xcb_button_press_event_t*)event;
                    bool buttonPressed = event->response_type == XCB_BUTTON_PRESS;
                    bool buttonClick = false;
                    bool scrolled = false;
                    switch (buttonEvent->detail) {
                        case XCB_BUTTON_INDEX_1: {
                            buttonClick = true;
                            GN_CORE_TRACE("Left Mouse Button {s}", buttonPressed ? "Pressed" : "Released");
                        } break;
                        case XCB_BUTTON_INDEX_2: {
                            buttonClick = true;
                            GN_CORE_TRACE("Middle Mouse Button {s}", buttonPressed ? "Pressed" : "Released");
                        } break;
                        case XCB_BUTTON_INDEX_3: {
                            buttonClick = true;
                            GN_CORE_TRACE("Right Mouse Button {s}", buttonPressed ? "Pressed" : "Released");
                        } break;
                        case XCB_BUTTON_INDEX_4: {
                            scrolled = true;
                            GN_CORE_TRACE("Middle Mouse Button Scrolled Up");
                        } break;
                        case XCB_BUTTON_INDEX_5: {
                            scrolled = true;
                            GN_CORE_TRACE("Middle Mouse Button Scrolled Down");
                        } break;
                    }
                    if (buttonClick) {
                        MouseButtonEvent mouseEvent(translateButton(buttonEvent->detail), buttonPressed);
                        EventSystem::fireEvent(mouseEvent);
                    } else if (scrolled) {
                        MouseScrollEvent scrollEvent(translateScroll(buttonEvent->detail));
                        EventSystem::fireEvent(scrollEvent);
                    }
                } break;
                case XCB_MOTION_NOTIFY: {
                    xcb_motion_notify_event_t* mouseEvent = (xcb_motion_notify_event_t*)event;
                    GN_CORE_TRACE("Mouse Moved: {i} {i}", mouseEvent->event_x, mouseEvent->event_y);
                    MouseMoveEvent moveEvent(mouseEvent->event_x, mouseEvent->event_y);
                    EventSystem::fireEvent(moveEvent);
                } break;
                case XCB_EXPOSE: {
                    GN_CORE_TRACE("Window Exposed");
                } break;
                case XCB_CLIENT_MESSAGE: {
                    xcb_client_message_event_t* clientMessage = (xcb_client_message_event_t*)event;

                    // Window close
                    if (clientMessage->data.data32[0] == m_windowManagerDeleteWindowAtom) {
                        WindowCloseEvent closeEvent;
                        EventSystem::fireEvent(closeEvent);
                    }
                } break;
                case XCB_CONFIGURE_NOTIFY: {
                    xcb_configure_notify_event_t* configureEvent = (xcb_configure_notify_event_t*)event;
                    if (configureEvent->width != m_windowWidth || configureEvent->height != m_windowHeight) {
                        GN_CORE_TRACE("Window Resized: {i} {i}", configureEvent->width, configureEvent->height);
                        m_windowWidth = configureEvent->width;
                        m_windowHeight = configureEvent->height;
                    }
                    if (configureEvent->x != m_x || configureEvent->y != m_y) {
                        GN_CORE_TRACE("Window Moved: {i} {i}", configureEvent->x, configureEvent->y);
                        m_x = configureEvent->x;
                        m_y = configureEvent->y;
                    }
                    WindowResizeEvent resizeEvent(configureEvent->width, configureEvent->height);
                    EventSystem::fireEvent(resizeEvent);
                } break;
                default:
                    break;
            }
        }
    }

    Key LinuxWindow::translateKey(xcb_keysym_t key) {
        switch (key) {
            case XK_BackSpace:
                return Key::KEY_BACKSPACE;
            case XK_Return:
            case XK_KP_Enter:
                return Key::KEY_ENTER;
            case XK_Tab:
                return Key::KEY_TAB;

            case XK_Pause:
                return Key::KEY_PAUSE;
            case XK_Caps_Lock:
                return Key::KEY_CAPITAL;

            case XK_Escape:
                return Key::KEY_ESCAPE;

            case XK_Mode_switch:
                return Key::KEY_MODECHANGE;

            case XK_space:
                return Key::KEY_SPACE;
            case XK_Prior:
                return Key::KEY_PAGEUP;
            case XK_Next:
                return Key::KEY_PAGEDOWN;
            case XK_End:
                return Key::KEY_END;
            case XK_Home:
                return Key::KEY_HOME;
            case XK_Left:
                return Key::KEY_LEFT;
            case XK_Up:
                return Key::KEY_UP;
            case XK_Right:
                return Key::KEY_RIGHT;
            case XK_Down:
                return Key::KEY_DOWN;
            case XK_Select:
                return Key::KEY_SELECT;
            case XK_Print:
                return Key::KEY_PRINT;
            case XK_Execute:
                return Key::KEY_EXECUTE;
            case XK_Insert:
                return Key::KEY_INSERT;
            case XK_Delete:
                return Key::KEY_DELETE;
            case XK_Help:
                return Key::KEY_HELP;

            case XK_Super_L:
                return Key::KEY_LSUPER;
            case XK_Super_R:
                return Key::KEY_RSUPER;

            case XK_KP_0:
            case XK_KP_Insert:
                return Key::KEY_NUMPAD0;
            case XK_KP_1:
            case XK_KP_End:
                return Key::KEY_NUMPAD1;
            case XK_KP_2:
            case XK_KP_Down:
                return Key::KEY_NUMPAD2;
            case XK_KP_3:
            case XK_KP_Page_Down:
                return Key::KEY_NUMPAD3;
            case XK_KP_4:
            case XK_KP_Left:
                return Key::KEY_NUMPAD4;
            case XK_KP_5:
            case XK_KP_Begin:
                return Key::KEY_NUMPAD5;
            case XK_KP_6:
            case XK_KP_Right:
                return Key::KEY_NUMPAD6;
            case XK_KP_7:
            case XK_KP_Home:
                return Key::KEY_NUMPAD7;
            case XK_KP_8:
            case XK_KP_Up:
                return Key::KEY_NUMPAD8;
            case XK_KP_9:
            case XK_KP_Page_Up:
                return Key::KEY_NUMPAD9;
            case XK_multiply:
            case XK_KP_Multiply:
                return Key::KEY_MULTIPLY;
            case XK_KP_Add:
                return Key::KEY_ADD;
            case XK_KP_Separator:
                return Key::KEY_SEPARATOR;
            case XK_KP_Subtract:
                return Key::KEY_SUBTRACT;
            case XK_KP_Decimal:
            case XK_KP_Delete:
                return Key::KEY_DECIMAL;
            case XK_KP_Divide:
                return Key::KEY_DIVIDE;
            case XK_F1:
                return Key::KEY_F1;
            case XK_F2:
                return Key::KEY_F2;
            case XK_F3:
                return Key::KEY_F3;
            case XK_F4:
                return Key::KEY_F4;
            case XK_F5:
                return Key::KEY_F5;
            case XK_F6:
                return Key::KEY_F6;
            case XK_F7:
                return Key::KEY_F7;
            case XK_F8:
                return Key::KEY_F8;
            case XK_F9:
                return Key::KEY_F9;
            case XK_F10:
                return Key::KEY_F10;
            case XK_F11:
                return Key::KEY_F11;
            case XK_F12:
                return Key::KEY_F12;
            case XK_F13:
                return Key::KEY_F13;
            case XK_F14:
                return Key::KEY_F14;
            case XK_F15:
                return Key::KEY_F15;
            case XK_F16:
                return Key::KEY_F16;
            case XK_F17:
                return Key::KEY_F17;
            case XK_F18:
                return Key::KEY_F18;
            case XK_F19:
                return Key::KEY_F19;
            case XK_F20:
                return Key::KEY_F20;
            case XK_F21:
                return Key::KEY_F21;
            case XK_F22:
                return Key::KEY_F22;
            case XK_F23:
                return Key::KEY_F23;
            case XK_F24:
                return Key::KEY_F24;

            case XK_Num_Lock:
                return Key::KEY_NUMLOCK;
            case XK_Scroll_Lock:
                return Key::KEY_SCROLL;

            case XK_KP_Equal:
                return Key::KEY_NUMPAD_EQUAL;

            case XK_Shift_L:
                return Key::KEY_LSHIFT;
            case XK_Shift_R:
                return Key::KEY_RSHIFT;
            case XK_Control_L:
                return Key::KEY_LCONTROL;
            case XK_Control_R:
                return Key::KEY_RCONTROL;
            case XK_Alt_L:
                return Key::KEY_LALT;
            case XK_Alt_R:
                return Key::KEY_RALT;

            case XK_semicolon:
                return Key::KEY_SEMICOLON;
            case XK_apostrophe:
                return Key::KEY_APOSTROPHE;
            case XK_plus:
            case XK_equal:
                return Key::KEY_EQUAL;
            case XK_comma:
                return Key::KEY_COMMA;
            case XK_minus:
                return Key::KEY_MINUS;
            case XK_period:
                return Key::KEY_PERIOD;
            case XK_slash:
                return Key::KEY_SLASH;
            case XK_grave:
                return Key::KEY_GRAVE;

            case XK_0:
                return Key::KEY_0;
            case XK_1:
                return Key::KEY_1;
            case XK_2:
                return Key::KEY_2;
            case XK_3:
                return Key::KEY_3;
            case XK_4:
                return Key::KEY_4;
            case XK_5:
                return Key::KEY_5;
            case XK_6:
                return Key::KEY_6;
            case XK_7:
                return Key::KEY_7;
            case XK_8:
                return Key::KEY_8;
            case XK_9:
                return Key::KEY_9;

            case XK_a:
            case XK_A:
                return Key::KEY_A;
            case XK_b:
            case XK_B:
                return Key::KEY_B;
            case XK_c:
            case XK_C:
                return Key::KEY_C;
            case XK_d:
            case XK_D:
                return Key::KEY_D;
            case XK_e:
            case XK_E:
                return Key::KEY_E;
            case XK_f:
            case XK_F:
                return Key::KEY_F;
            case XK_g:
            case XK_G:
                return Key::KEY_G;
            case XK_h:
            case XK_H:
                return Key::KEY_H;
            case XK_i:
            case XK_I:
                return Key::KEY_I;
            case XK_j:
            case XK_J:
                return Key::KEY_J;
            case XK_k:
            case XK_K:
                return Key::KEY_K;
            case XK_l:
            case XK_L:
                return Key::KEY_L;
            case XK_m:
            case XK_M:
                return Key::KEY_M;
            case XK_n:
            case XK_N:
                return Key::KEY_N;
            case XK_o:
            case XK_O:
                return Key::KEY_O;
            case XK_p:
            case XK_P:
                return Key::KEY_P;
            case XK_q:
            case XK_Q:
                return Key::KEY_Q;
            case XK_r:
            case XK_R:
                return Key::KEY_R;
            case XK_s:
            case XK_S:
                return Key::KEY_S;
            case XK_t:
            case XK_T:
                return Key::KEY_T;
            case XK_u:
            case XK_U:
                return Key::KEY_U;
            case XK_v:
            case XK_V:
                return Key::KEY_V;
            case XK_w:
            case XK_W:
                return Key::KEY_W;
            case XK_x:
            case XK_X:
                return Key::KEY_X;
            case XK_y:
            case XK_Y:
                return Key::KEY_Y;
            case XK_z:
            case XK_Z:
                return Key::KEY_Z;

            case XK_bracketleft:
                return Key::KEY_LBRACKET;
            case XK_backslash:
                return Key::KEY_BACKSLASH;
            case XK_bracketright:
                return Key::KEY_RBRACKET;

            default:
                return Key::KEYS_MAX_KEYS;
        }
    }

    Button LinuxWindow::translateButton(xcb_button_t button) {
        switch (button) {
            case XCB_BUTTON_INDEX_1:
                return Button::BUTTON_LEFT;
            case XCB_BUTTON_INDEX_2:
                return Button::BUTTON_MIDDLE;
            case XCB_BUTTON_INDEX_3:
                return Button::BUTTON_RIGHT;

            default:
                return Button::BUTTON_MAX_BUTTONS;
        }
    }

    int16_t LinuxWindow::translateScroll(xcb_button_t button) {
        switch (button) {
            case XCB_BUTTON_INDEX_4:
                return -1;
            case XCB_BUTTON_INDEX_5:
                return 1;

            default:
                return 0;
        }
    }
}  // namespace Genesis
