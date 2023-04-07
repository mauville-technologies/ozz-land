//
// Created by Paul Mauviel on 2023-04-06.
//

#include <ozz_vulkan/renderer.h>
#include <spdlog/spdlog.h>
#include <vector>

namespace OZZ_VULKAN {
    void Renderer::Init() {
        spdlog::info("Initializing Renderer.");
        initInstance();
    }

    Renderer::~Renderer() {
        spdlog::info("Destroying Renderer");
        destroyInstance();
    }

    void Renderer::destroyInstance() {
        if (_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(_instance, nullptr);
            spdlog::info("Vulkan Instance Destroyed");
        }
    }

    void Renderer::initInstance() {
        VkApplicationInfo applicationInfo { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        applicationInfo.apiVersion = VK_API_VERSION_1_3;
        applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationInfo.pApplicationName = "No Application Name";
        applicationInfo.pEngineName = "No Engine Name";



        VkInstanceCreateInfo instanceCreateInfo { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instanceCreateInfo.pApplicationInfo = &applicationInfo;

        std::vector<const char*> extNames;

        extNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#ifdef __APPLE__
        extNames.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        instanceCreateInfo.flags = instanceCreateInfo.flags | VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        // Enabled extensions, we'll keep empty for now
        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extNames.size());
        instanceCreateInfo.ppEnabledExtensionNames = extNames.data();

        // Validation layers
        instanceCreateInfo.enabledLayerCount = 0;
        auto result = vkCreateInstance(&instanceCreateInfo, nullptr, &_instance);
        if (result != VK_SUCCESS) {
            spdlog::error("Failed to create vulkan instance {}", result);
            return;
        }
        spdlog::info("Vulkan instance Created");
    }
}