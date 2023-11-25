add_library(genesis
    STATIC
    include/Genesis.h
    src/Core/Application.cpp src/Core/Application.h
    src/Core/Entry.h
    src/Core/EventSystem.cpp src/Core/EventSystem.h
    src/Core/Logger.cpp src/Core/Logger.h
    src/Core/Window.cpp src/Core/Window.h
    src/Events/Event.h
    src/Events/ApplicationEvents.h
    src/Events/KeyboardEvents.h
    src/Events/MouseEvents.h
    src/Platform/LinuxWindow.cpp src/Platform/LinuxWindow.h
    src/Platform/PlatformDetection.h
)

target_link_libraries(genesis quill xcb xcb-util xcb-keysyms)

target_include_directories(genesis
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${quill_SOURCE_DIR}/quill/include
)

target_compile_options(genesis PRIVATE -Werror)
target_compile_features(genesis PRIVATE cxx_std_20)
target_precompile_headers(genesis
    PRIVATE
        <string>
        <memory>
        <quill/Quill.h>
)
