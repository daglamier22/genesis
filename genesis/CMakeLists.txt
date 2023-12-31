add_library(genesis
    STATIC
    include/Genesis.h
    src/Core/Application.cpp src/Core/Application.h
    src/Core/Entry.h
    src/Core/EventSystem.cpp src/Core/EventSystem.h
    src/Core/InputSystem.cpp src/Core/InputSystem.h
    src/Core/Keyboard.cpp src/Core/Keyboard.h
    src/Core/Mouse.cpp src/Core/Mouse.h
    src/Core/Logger.cpp src/Core/Logger.h
    src/Core/Scene.cpp src/Core/Scene.h
    src/Core/Window.cpp src/Core/Window.h
    src/Events/Event.h
    src/Events/ApplicationEvents.h
    src/Events/KeyboardEvents.h
    src/Events/MouseEvents.h
    src/Platform/LinuxWindow.cpp src/Platform/LinuxWindow.h
    src/Platform/GLFWWindow.cpp src/Platform/GLFWWindow.h
    src/Platform/PlatformDetection.h
    src/Renderer/Vulkan/VulkanTypes.h
    src/Renderer/Vulkan/VulkanRenderer.cpp src/Renderer/Vulkan/VulkanRenderer.h
    src/Renderer/Vulkan/VulkanDevice.cpp src/Renderer/Vulkan/VulkanDevice.h
    src/Renderer/Vulkan/VulkanSwapchain.cpp src/Renderer/Vulkan/VulkanSwapchain.h
    src/Renderer/Vulkan/VulkanImage.cpp src/Renderer/Vulkan/VulkanImage.h
    src/Renderer/Vulkan/VulkanShader.cpp src/Renderer/Vulkan/VulkanShader.h
    src/Renderer/Vulkan/VulkanPipeline.cpp src/Renderer/Vulkan/VulkanPipeline.h
    src/Renderer/Vulkan/VulkanMesh.cpp src/Renderer/Vulkan/VulkanMesh.h
    src/Renderer/Vulkan/VulkanBuffer.cpp src/Renderer/Vulkan/VulkanBuffer.h
    src/Renderer/Vulkan/VulkanVertexMenagerie.cpp src/Renderer/Vulkan/VulkanVertexMenagerie.h
    src/Renderer/Vulkan/VulkanTexture.cpp src/Renderer/Vulkan/VulkanTexture.h
    src/Renderer/Vulkan/VulkanCommandBuffer.cpp src/Renderer/Vulkan/VulkanCommandBuffer.h
    src/Resources/ObjMesh.cpp src/Resources/ObjMesh.h
    src/Resources/Utils.cpp src/Resources/Utils.h
)

target_link_libraries(genesis quill xcb xcb-util xcb-keysyms glfw glm vulkan)

target_include_directories(genesis
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${quill_SOURCE_DIR}/quill/include
        ${glfw_SOURCE_DIR}/include
        ${glm_SOURCE_DIR}
        ${Vulkan_INCLUDE_DIR}
        ${stb_SOURCE_DIR}
        ${tinyobjloader_SOURCE_DIR}
)

target_compile_options(genesis PRIVATE -Werror)
target_compile_features(genesis PRIVATE cxx_std_20)
target_precompile_headers(genesis
    PRIVATE
        <string>
        <vector>
        <memory>
        <set>
        <optional>
        <stdexcept>
        <quill/Quill.h>
        <vulkan/vulkan.h>
)
