#include "VulkanGraphics.h"

namespace vvoxelen{
    VulkanGraphics* VulkanGraphics::singleton = nullptr;

    VulkanGraphics::VulkanGraphics() : instance(VK_NULL_HANDLE), surface(VK_NULL_HANDLE)
    {
        assert(singleton == nullptr);
        singleton = this;
    }

    VulkanGraphics::~VulkanGraphics()
    {
        // Clean up.
        if (semaphoreData.presentComplete != VK_NULL_HANDLE)
        {
			vkDestroySemaphore(deviceData.device, semaphoreData.presentComplete, nullptr);
        }
        if (semaphoreData.renderComplete != VK_NULL_HANDLE)
        {
			vkDestroySemaphore(deviceData.device, semaphoreData.renderComplete, nullptr);
        }
        if (deviceData.commandPool != VK_NULL_HANDLE)
        {
			vkDestroyCommandPool(deviceData.device, deviceData.commandPool, nullptr);
        }
        if (deviceData.device != VK_NULL_HANDLE) 
        {
            vkDestroyDevice(deviceData.device, nullptr);
        }

        if (surface != VK_NULL_HANDLE) 
        {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }

        if (instance != VK_NULL_HANDLE) 
        {
            vkDestroyInstance(instance, nullptr);
        }
    }

    void VulkanGraphics::initVulkan()
    {
        createInstance();

        uint32_t gpuCount = 0;
		vkEnumeratePhysicalDevices(instance, &gpuCount, NULL);

        if (gpuCount == 0) {
			std::cout << "Could not find a GPU with Vulkan support." << std::endl;
            exit(1);
        }
        
        std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
		VkResult result = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
        if (result != VK_SUCCESS)
        {
            std::cout << "Could not enumerate physical devices." << std::endl;
			exit(1);
        }

        // Create a Vulkan surface for rendering
        if (!Window::getSingleton()->CreateVulkanSurface(instance, surface)) {
            std::cout << "Could not create a Vulkan surface." << std::endl;
            exit(1);
        }

        if (gpuCount > 1) {
            // TODO: Handle multiple GPUs
        }
        physicalDeviceData.device = physicalDevices[0];

        vkGetPhysicalDeviceProperties(physicalDeviceData.device, &physicalDeviceData.Properties);
        vkGetPhysicalDeviceFeatures(physicalDeviceData.device, &physicalDeviceData.Features);
        vkGetPhysicalDeviceMemoryProperties(physicalDeviceData.device, &physicalDeviceData.MemoryProperties);

        uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceData.device, &queueFamilyCount, NULL);
        physicalDeviceData.QueueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceData.device, &queueFamilyCount, physicalDeviceData.QueueFamilyProperties.data());

        createPhysicalDevice();

        vkGetDeviceQueue(deviceData.device, deviceData.queueFamilyIndices.graphics, 0, &deviceData.queue);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        vkCreateSemaphore(deviceData.device, &semaphoreInfo, NULL, &semaphoreData.presentComplete);
        vkCreateSemaphore(deviceData.device, &semaphoreInfo, NULL, &semaphoreData.renderComplete);

        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask = &pipelineStageFlags;
        submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &semaphoreData.presentComplete;
        submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphoreData.renderComplete;
    }

    void VulkanGraphics::createInstance()
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
        for (const char* extension : extensions) {
            if (std::find(instanceExtensions.begin(), instanceExtensions.end(), extension) == instanceExtensions.end()) {
				instanceExtensions.push_back(extension);
            }
        }
        // VkInstanceCreateInfo is where the programmer specifies the layers and/or extensions that
        // are needed.

        unsigned int extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

        std::vector<VkExtensionProperties> extensionsProperties(extensionCount);
        VkResult result = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensionsProperties.data());
        
        if (result != VK_SUCCESS) {
            std::cout << "Could not enumerate extensions." << std::endl;
		    exit(1);
        }
        for (const VkExtensionProperties& extension : extensionsProperties) {
            if (std::find(instanceExtensions.begin(), instanceExtensions.end(), extension.extensionName) == instanceExtensions.end()) {
                instanceExtensions.push_back(extension.extensionName);
            }
        }

        VkInstanceCreateInfo instInfo = {};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pNext = NULL;
        instInfo.flags = 0;
        instInfo.pApplicationInfo = &appInfo;
        instInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        instInfo.ppEnabledExtensionNames = instanceExtensions.data();
        instInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        instInfo.ppEnabledLayerNames = layers.data();

        // Create the Vulkan instance.

        result = vkCreateInstance(&instInfo, NULL, &instance);
        if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
            std::cout << "Unable to find a compatible Vulkan Driver." << std::endl;
            exit(1);
        }
        else if (result) {
            std::cout << "Could not create a Vulkan instance (for unknown reasons)." << std::endl;
            exit(1);
        }
    }

    void VulkanGraphics::createPhysicalDevice()
    {
        deviceData.queueFamilyIndices.graphics = getGraphicsQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
        // TODO: Handle multiple GPUs, using the first one for now
        deviceData.queueFamilyIndices.compute = deviceData.queueFamilyIndices.graphics;
        deviceData.queueFamilyIndices.transfer = deviceData.queueFamilyIndices.graphics;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = deviceData.queueFamilyIndices.graphics;
        queueCreateInfo.queueCount = 1;
        const float queuePriority{ 0.0f };
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);

        std::vector<const char*> extensions;
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        deviceData.deviceExtensions = extensions;

        VkDeviceCreateInfo devInfo = {};
		devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        devInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        devInfo.pQueueCreateInfos = queueCreateInfos.data();

        devInfo.enabledExtensionCount = static_cast<uint32_t>(deviceData.deviceExtensions.size());
        devInfo.ppEnabledExtensionNames = deviceData.deviceExtensions.data();

        VkResult result = vkCreateDevice(physicalDeviceData.device, &devInfo, NULL, &deviceData.device);
        if (result != VK_SUCCESS) {
			std::cout << "Could not create a Vulkan device (for unknown reasons)." << result << std::endl;
            exit(1);
        }
        deviceData.commandPool = createCommandPool(deviceData.queueFamilyIndices.graphics);
    }

    VkCommandPool VulkanGraphics::createCommandPool(int32_t queueFamilyIndex)
    {
        VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VkCommandPool commandPool;
        vkCreateCommandPool(deviceData.device, &poolInfo, NULL, &commandPool);
        return commandPool;
    }

    int32_t VulkanGraphics::getGraphicsQueueFamilyIndex(VkQueueFlagBits queueFlags)
    {
        if (queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            for (uint32_t i = 0; i < physicalDeviceData.QueueFamilyProperties.size(); i++)
            {
				if (physicalDeviceData.QueueFamilyProperties[i].queueFlags & queueFlags)
                    return i;
            }
        }
        return 0;
    }

    VulkanGraphics* VulkanGraphics::GetSingleton()
    {
        return singleton;
    }
}
