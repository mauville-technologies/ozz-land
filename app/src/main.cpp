//
// Created by Paul Mauviel on 2023-04-06.
//

#include <memory>
#include "application.h"
#include <thread>
#include <iostream>

int main(int argc, char** argv) {
    std::unique_ptr<Application> application = std::make_unique<Application>();

    std::thread appThread([&]() {
        application->Run();
        spdlog::info("Application thread exited");
    });

    // loop until escape character is pressed
    while (std::cin.get() != 27) {
       // do nothing
    }

    application->Stop();
    appThread.join();
    application.reset(nullptr);
    spdlog::info("Application exited");
}