#pragma once

namespace Genesis {
    struct WindowCreationProperties {
            std::string title;
            int16_t x;
            int16_t y;
            uint16_t width;
            uint16_t height;

            WindowCreationProperties() : title("Genesis Engine"),
                                         x(500),
                                         y(500),
                                         width(800),
                                         height(600) {}
    };

    class Window {
        public:
            virtual bool onUpdate() = 0;

            static std::unique_ptr<Window> create(const WindowCreationProperties properties = WindowCreationProperties());
    };
}  // namespace Genesis
