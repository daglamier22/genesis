cmake_minimum_required(VERSION 3.24.0)

include(FetchContent)
# fetch quill logger
FetchContent_Declare(quill
    URL https://github.com/odygrd/quill/archive/refs/tags/v3.5.1.zip
    URL_HASH SHA256=c0f641b71f62abd0efe2b115725e97eac90715b64d0159c83b30775e493526c8
)
# fetch glfw for window creation
FetchContent_Declare(glfw
    URL https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.zip
    URL_HASH SHA256=4d025083cc4a3dd1f91ab9b9ba4f5807193823e565a5bcf4be202669d9911ea6
)
# fetch glm for shader math
FetchContent_Declare(glm
    URL https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip
    URL_HASH SHA256=37e2a3d62ea3322e43593c34bae29f57e3e251ea89f4067506c94043769ade4c
)
# fetch vulkan sdk and tools
# FetchContent_Declare(vulkan
#     URL https://sdk.lunarg.com/sdk/download/1.3.268.0/linux/vulkansdk-linux-x86_64-1.3.268.0.tar.xz
#     URL_HASH SHA256=d23343736247828ff5b3b6b1b7fd83a72b5df1a54b2527ae3663cebdfee75842
# )
FetchContent_MakeAvailable(quill glfw glm)
find_package(Vulkan REQUIRED)

project(genesis VERSION 0.1.0 LANGUAGES C CXX)
add_subdirectory(genesis genesis)

project(testbed VERSION 0.1.0 LANGUAGES C CXX)
add_subdirectory(testbed testbed)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
