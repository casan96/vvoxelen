#include "VulkanGraphics.h"

vvoxelen::VulkanGraphics* vvoxelen::VulkanGraphics::singleton = nullptr;

vvoxelen::VulkanGraphics::VulkanGraphics() : instance(VK_NULL_HANDLE), surface(VK_NULL_HANDLE)
{
    assert(singleton == nullptr);
    singleton = this;
}

vvoxelen::VulkanGraphics::~VulkanGraphics()
{
    // Clean up.
    vkDestroySurfaceKHR(instance, surface, NULL);

    vkDestroyInstance(instance, NULL);
}

void vvoxelen::VulkanGraphics::initVulkan()
{
	createInstance();
	createSurface();
}

void vvoxelen::VulkanGraphics::createInstance()
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
    std::string appName = Window::getSingleton()->getTitle();
    appInfo.pApplicationName = appName.c_str();
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "VoxelEngine";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> extensions = Window::getSingleton()->GetSDLExtensions();
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

void vvoxelen::VulkanGraphics::createSurface()
{
    // Create a Vulkan surface for rendering
    if (!Window::getSingleton()->CreateVulkanSurface(instance, surface)) {
        std::cout << "Could not create a Vulkan surface." << std::endl;
        exit(1);
    }
}

vvoxelen::VulkanGraphics* vvoxelen::VulkanGraphics::GetSingleton()
{
    return singleton;
}
