add_library(genesis
    STATIC
    include/genesis.h
    src/entry.h
    src/application.cpp src/application.h
    src/Core/Logger.cpp src/Core/Logger.h
)

target_link_libraries(genesis quill)

target_include_directories(genesis
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${quill_SOURCE_DIR}/quill/include
)

target_compile_options(genesis PRIVATE -Werror)
target_compile_features(genesis PRIVATE cxx_std_20)
target_compile_definitions(genesis PUBLIC GN_QUILL_LOGGER)
