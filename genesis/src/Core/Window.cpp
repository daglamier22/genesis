#include "Core/Window.h"

#include <cstdlib>

#include "Core/Logger.h"
#include "Platform/PlatformDetection.h"
#ifdef GN_PLATFORM_LINUX
    #include "Platform/GLFWWindow.h"
    #include "Platform/LinuxWindow.h"
#endif

namespace Genesis {
    std::shared_ptr<Window> Window::create(const WindowCreationProperties properties) {
#ifdef GN_PLATFORM_LINUX
        return std::make_unique<GLFWWindow>(properties);
#else
        GN_CORE_CRITICAL("Unsupported platform.");
        return nullptr;
#endif
    }
}  // namespace Genesis
