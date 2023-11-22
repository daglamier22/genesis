set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/)

add_executable(testbed
    src/main.cpp
)

target_include_directories(testbed
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(testbed
    PUBLIC
        genesis
)

target_compile_options(testbed PRIVATE -Werror)
target_compile_features(testbed PRIVATE cxx_std_20)
target_precompile_headers(testbed
    PRIVATE
        <string>
        <quill/Quill.h>
)
