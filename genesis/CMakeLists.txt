set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/)

add_library(genesis
    SHARED
    include/entry.h
    src/application.cpp src/application.h
)

target_include_directories(genesis
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
# target_link_libraries(genesis
#     PUBLIC
# )

target_compile_options(genesis PRIVATE -Werror)
target_compile_features(genesis PRIVATE cxx_std_17)
