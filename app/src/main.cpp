//
// Created by Paul Mauviel on 2023-04-06.
//

#include <spdlog/spdlog.h>
#include <ozz_vulkan/renderer.h>

int main(int argc, char* argv[]) {
    spdlog::info("Initializing Application.");

    OZZ_VULKAN::Renderer myRenderer {};
    myRenderer.Init();
}