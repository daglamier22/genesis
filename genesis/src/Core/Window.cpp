#include "Core/Window.h"

#include "Core/Logger.h"
#include "Platform/PlatformDetection.h"
#ifdef GN_PLATFORM_LINUX
    #include "Platform/LinuxWindow.h"
#endif

namespace Genesis {
    std::unique_ptr<Window> Window::create(const WindowCreationProperties properties) {
#ifdef GN_PLATFORM_LINUX
        return std::make_unique<LinuxWindow>(properties);
#else
        GN_CORE_CRITICAL("Unsupported platform.");
        return nullptr;
#endif
    }
}  // namespace Genesis
