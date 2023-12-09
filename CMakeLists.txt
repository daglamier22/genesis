cmake_minimum_required(VERSION 3.24.0)

include(FetchContent)
FetchContent_Declare(quill
    GIT_REPOSITORY https://github.com/odygrd/quill
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(quill)

project(genesis VERSION 0.1.0 LANGUAGES C CXX)
add_subdirectory(genesis genesis)

project(testbed VERSION 0.1.0 LANGUAGES C CXX)
add_subdirectory(testbed testbed)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
