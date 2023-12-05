#include "Mouse.h"

namespace Genesis {
    std::string buttonToString(Button button) {
        switch (button) {
            case Button::BUTTON_LEFT:
                return "Button::BUTTON_LEFT";
            case Button::BUTTON_RIGHT:
                return "Button::BUTTON_RIGHT";
            case Button::BUTTON_MIDDLE:
                return "Button::BUTTON_MIDDLE";

            default:
                return "Unknown Button";
        }
    }
}  // namespace Genesis
