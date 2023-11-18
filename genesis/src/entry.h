#pragma once

extern Genesis::Application* Genesis::createApp();

int main(int argc, char** argv) {
    auto app = Genesis::createApp();
    app->run();
    delete app;
}
