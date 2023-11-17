#pragma once

#include <iostream>
#include <string>

extern std::string createApp();

int main(int argc, char** argv) {
    std::string gameString = createApp();
    std::cout << "Hello from genesis and " << gameString << std::endl;
}
