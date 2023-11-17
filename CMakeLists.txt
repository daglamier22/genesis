cmake_minimum_required(VERSION 3.24.0)

project(genesis VERSION 0.1.0 LANGUAGES C CXX)
add_subdirectory(genesis genesis)

project(testbed VERSION 0.1.0 LANGUAGES C CXX)
add_subdirectory(testbed testbed)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)