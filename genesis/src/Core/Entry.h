#pragma once

#include <cstdlib>
#include <iostream>

#include "Application.h"
#include "Logger.h"

extern Genesis::Application* Genesis::createApp();

int main(int argc, char** argv) {
    try {
        auto app = Genesis::createApp();
        GN_CORE_INFO("Application created.");
        app->run();
        delete app;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
