#include "VulkanRender.h"

vvoxelen::VulkanRender::VulkanRender(Window* currentWindow)
{
    vvoxWindow = currentWindow;
    createInstance();
	createSurface();
}

vvoxelen::VulkanRender::~VulkanRender()
{
    // Clean up.
    vkDestroySurfaceKHR(instance, surface, NULL);

    vkDestroyInstance(instance, NULL);
}

void vvoxelen::VulkanRender::createInstance()
{
    // Use validation layers if this is a debug build
    std::vector<const char*> layers;
#if defined(_DEBUG)
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    // VkApplicationInfo allows the programmer to specifiy some basic information about the
    // program, which can be useful for layers and tools to provide more debug information.
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
    appInfo.pApplicationName = "Vulkan Program Template";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "LunarG SDK";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> extensions = vvoxWindow->GetSDLExtensions();
    // VkInstanceCreateInfo is where the programmer specifies the layers and/or extensions that
    // are needed.
    VkInstanceCreateInfo instInfo = {};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pNext = NULL;
    instInfo.flags = 0;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instInfo.ppEnabledExtensionNames = extensions.data();
    instInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    instInfo.ppEnabledLayerNames = layers.data();

    // Create the Vulkan instance.
    
    VkResult result = vkCreateInstance(&instInfo, NULL, &instance);
    if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
        std::cout << "Unable to find a compatible Vulkan Driver." << std::endl;
        exit(1);
    }
    else if (result) {
        std::cout << "Could not create a Vulkan instance (for unknown reasons)." << std::endl;
        exit(1);
    }
}

void vvoxelen::VulkanRender::createSurface()
{
    // Create a Vulkan surface for rendering
    if (!vvoxWindow->CreateVulkanSurface(instance, surface)) {
        std::cout << "Could not create a Vulkan surface." << std::endl;
        exit(1);
    }
}
