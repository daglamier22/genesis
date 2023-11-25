#pragma once

#include "Application.h"
#include "Logger.h"

extern Genesis::Application* Genesis::createApp();

int main(int argc, char** argv) {
    auto app = Genesis::createApp();
    GN_CORE_INFO("Application created.");
    app->run();
    delete app;
}
