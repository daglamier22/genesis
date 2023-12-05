#pragma once

namespace Genesis {
    enum class Button {
        BUTTON_LEFT,
        BUTTON_RIGHT,
        BUTTON_MIDDLE,
        BUTTON_MAX_BUTTONS
    };

    std::string buttonToString(Button button);
}  // namespace Genesis

#define BUTTONTOSTR(b) (::Genesis::buttonToString(b))
