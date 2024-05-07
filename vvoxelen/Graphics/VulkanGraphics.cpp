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
        if (swapchainData.swapchain != VK_NULL_HANDLE)
        {
            for (uint32_t i = 0; i < swapchainData.imageCount; i++)
            {
                vkDestroyImageView(deviceData.device, swapchainData.framebuffers[i].view, nullptr);
            }
			vkDestroySwapchainKHR(deviceData.device, swapchainData.swapchain, nullptr);
        }
        vkFreeCommandBuffers(deviceData.device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        if (renderPass != VK_NULL_HANDLE)
        {
			vkDestroyRenderPass(deviceData.device, renderPass, nullptr);
        }
        for (uint32_t i = 0; i < framebuffers.size(); i++)
        {
			vkDestroyFramebuffer(deviceData.device, framebuffers[i], nullptr);
        }
        vkDestroyImageView(deviceData.device, depthStencilData.View, nullptr);
		vkDestroyImage(deviceData.device, depthStencilData.Image, nullptr);
		vkFreeMemory(deviceData.device, depthStencilData.Memory, nullptr);

        vkDestroyPipelineCache(deviceData.device, pipelineCache, nullptr);
        for (auto& fence : fences)
        {
			vkDestroyFence(deviceData.device, fence, nullptr);
        }
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
        if (commandPool != VK_NULL_HANDLE)
        {
			vkDestroyCommandPool(deviceData.device, commandPool, nullptr);
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

        createLogicalDevice();

        vkGetDeviceQueue(deviceData.device, deviceData.queueFamilyIndices.graphics, 0, &deviceData.queue);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        vkCreateSemaphore(deviceData.device, &semaphoreInfo, NULL, &semaphoreData.presentComplete);
        vkCreateSemaphore(deviceData.device, &semaphoreInfo, NULL, &semaphoreData.renderComplete);

        std::vector<VkFormat> formatList = {
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM
        };

        for (auto& format : formatList)
        {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(physicalDeviceData.device, format, &formatProps);
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                depthStencilData.Format = format;
                break;
            }
        }

        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask = &pipelineStageFlags;
        submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &semaphoreData.presentComplete;
        submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphoreData.renderComplete;

        initSwapchain();
        createCommandPool();
        setupSwapchain();
        CreateCommandBuffers();
        CreateFences();
        SetupDepthStencil();
        CreateRenderPass();

        VkPipelineCacheCreateInfo pipelineCacheInfo = {};
		pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        assert(vkCreatePipelineCache(deviceData.device, &pipelineCacheInfo, nullptr, &pipelineCache) == VK_SUCCESS);

        VkImageView attachments[2];
        attachments[1] = depthStencilData.View;

        VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = NULL;
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = 2;
		framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = Window::getSingleton()->getWidth();
		framebufferCreateInfo.height = Window::getSingleton()->getHeight();
		framebufferCreateInfo.layers = 1;

        framebuffers.resize(swapchainData.imageCount);
        for (uint32_t i = 0; i < swapchainData.imageCount; i++)
        {
			attachments[0] = swapchainData.framebuffers[i].view;
            assert(vkCreateFramebuffer(deviceData.device, &framebufferCreateInfo, nullptr, &framebuffers[i]) == VK_SUCCESS);
        }

    }

    void VulkanGraphics::initSwapchain()
    {
        std::vector<VkBool32> presentSuport(physicalDeviceData.QueueFamilyProperties.size());
        for (uint32_t i = 0; i < physicalDeviceData.QueueFamilyProperties.size(); i++)
        {
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDeviceData.device, i, surface, &presentSuport[i]);
        }

        uint32_t graphicsQueueNodeIndex = UINT32_MAX;
        uint32_t presentQueueNodeIndex = UINT32_MAX;
        for (uint32_t i = 0; i < physicalDeviceData.QueueFamilyProperties.size(); i++)
        {
            if ((physicalDeviceData.QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
            {
                if (graphicsQueueNodeIndex == UINT32_MAX)
                {
                    graphicsQueueNodeIndex = i;
                }

                if (presentSuport[i] == VK_TRUE)
                {
                    graphicsQueueNodeIndex = i;
                    presentQueueNodeIndex = i;
                    break;
                }
            }
        }
        if (presentQueueNodeIndex == UINT32_MAX)
        {
            // If there's no queue that supports both present and graphics
            // try to find a separate present queue
            for (uint32_t i = 0; i < physicalDeviceData.QueueFamilyProperties.size(); ++i)
            {
                if (presentSuport[i] == VK_TRUE)
                {
                    presentQueueNodeIndex = i;
                    break;
                }
            }
        }
        if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
        {
			std::cout << "Could not find a graphics and present queue" << std::endl;
			exit(1);
        }
        else if (graphicsQueueNodeIndex != presentQueueNodeIndex)
        {
			std::cout << "Graphics and present queues are not on the same queue node" << std::endl;
			exit(1);
        }
        swapchainData.queueNodeIndex = graphicsQueueNodeIndex;
        
        uint32_t formatCount = 0;
        assert(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDeviceData.device, surface, &formatCount, nullptr) == VK_SUCCESS);
		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		assert(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDeviceData.device, surface, &formatCount, surfaceFormats.data()) == VK_SUCCESS);

        VkSurfaceFormatKHR selectedFormat =surfaceFormats[0];

        std::vector<VkFormat> preferredImageFormats = {
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_FORMAT_A8B8G8R8_UNORM_PACK32
        };

        for (auto& availableFormat : surfaceFormats) {
            if (std::find(preferredImageFormats.begin(), preferredImageFormats.end(), availableFormat.format) != preferredImageFormats.end()) {
                selectedFormat = availableFormat;
                break;
            }
        }

        swapchainData.colorFormat = selectedFormat.format;
        swapchainData.colorSpace = selectedFormat.colorSpace;
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

    void VulkanGraphics::createLogicalDevice()
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

    void VulkanGraphics::createCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = deviceData.queueFamilyIndices.graphics;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        assert(vkCreateCommandPool(deviceData.device, &poolInfo, nullptr, &commandPool) == VK_SUCCESS);
    }

    void VulkanGraphics::CreateCommandBuffers()
    {
        commandBuffers.resize(swapchainData.imageCount);
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        assert(vkAllocateCommandBuffers(deviceData.device, &allocInfo, commandBuffers.data()) == VK_SUCCESS);
    }

    void VulkanGraphics::CreateFences()
    {
		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fences.resize(commandBuffers.size());
        for (auto& fence : fences)
        {
			assert(vkCreateFence(deviceData.device, &fenceInfo, nullptr, &fence) == VK_SUCCESS);
        }
    }

    void VulkanGraphics::setupSwapchain()
    {
        VkSwapchainKHR oldSwapchain = swapchainData.swapchain;
        // TODO: Handle multiple GPUs
        swapchainData.queueNodeIndex = deviceData.queueFamilyIndices.graphics;

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDeviceData.device, surface, &capabilities);

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceData.device, surface, &presentModeCount, nullptr);

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceData.device, surface, &presentModeCount, presentModes.data());

        VkExtent2D swapchainExtent = {};
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            swapchainExtent.width = Window::getSingleton()->getWidth();
            swapchainExtent.height = Window::getSingleton()->getHeight();
        }
        else
        {
            swapchainExtent = capabilities.currentExtent;
            Window::getSingleton()->setWidth(swapchainExtent.width);
            Window::getSingleton()->setHeight(swapchainExtent.height);
        }
        // TODO: Handle no use of v-sync
        VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        {
            imageCount = capabilities.maxImageCount;
        }

        VkSurfaceTransformFlagBitsKHR preTransform;
        if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            preTransform = capabilities.currentTransform;
        }

        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        };

        for (auto& compositeAlphaFlag : compositeAlphaFlags)
        {
            if (capabilities.supportedCompositeAlpha & compositeAlphaFlag)
            {
                compositeAlpha = compositeAlphaFlag;
                break;
            }
        }

        VkSwapchainCreateInfoKHR swapchainInfo = {};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = surface;
        swapchainInfo.minImageCount = imageCount;
        swapchainInfo.imageFormat = swapchainData.colorFormat;
        swapchainInfo.imageColorSpace = swapchainData.colorSpace;
        swapchainInfo.imageExtent = swapchainExtent;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.preTransform = preTransform;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0;
        swapchainInfo.presentMode = swapchainPresentMode;
        swapchainInfo.oldSwapchain = oldSwapchain;
        swapchainInfo.compositeAlpha = compositeAlpha;
        swapchainInfo.clipped = VK_TRUE;

        if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        {
			swapchainInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        {
			swapchainInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

		assert(vkCreateSwapchainKHR(deviceData.device, &swapchainInfo, nullptr, &swapchainData.swapchain)==VK_SUCCESS);

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            for (uint32_t i = 0; i < swapchainData.imageCount; i++)
            {
				vkDestroyImageView(deviceData.device, swapchainData.framebuffers[i].view, nullptr);
            }
			vkDestroySwapchainKHR(deviceData.device, oldSwapchain, nullptr);
        }

        assert(vkGetSwapchainImagesKHR(deviceData.device, swapchainData.swapchain, &swapchainData.imageCount, nullptr) == VK_SUCCESS);

		swapchainData.images.resize(swapchainData.imageCount);

        assert(vkGetSwapchainImagesKHR(deviceData.device, swapchainData.swapchain, &swapchainData.imageCount, swapchainData.images.data()) == VK_SUCCESS);

        swapchainData.framebuffers.resize(swapchainData.imageCount);
        for (uint32_t i = 0; i < swapchainData.imageCount; i++)
        {
            VkImageViewCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageInfo.pNext = NULL;
            imageInfo.format = swapchainData.colorFormat;
            imageInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			imageInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageInfo.subresourceRange.baseMipLevel = 0;
			imageInfo.subresourceRange.levelCount = 1;
			imageInfo.subresourceRange.baseArrayLayer = 0;
			imageInfo.subresourceRange.layerCount = 1;
            imageInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

            swapchainData.framebuffers[i].image = swapchainData.images[i];
			imageInfo.image = swapchainData.images[i];

            assert(vkCreateImageView(deviceData.device, &imageInfo, nullptr, &swapchainData.framebuffers[i].view) == VK_SUCCESS);
        }
    }

    void VulkanGraphics::SetupDepthStencil()
    {
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = depthStencilData.Format;
        imageInfo.extent = { static_cast<uint32_t>(Window::getSingleton()->getWidth()), static_cast<uint32_t>(Window::getSingleton()->getHeight()), 1};
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        assert(vkCreateImage(deviceData.device, &imageInfo, nullptr, &depthStencilData.Image) == VK_SUCCESS);

        VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(deviceData.device, depthStencilData.Image, &memRequirements);

        VkMemoryAllocateInfo memAlloc = {};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memRequirements.size;

        uint32_t memoryTypeIndex = 0;
        for (uint32_t i = 0; i < physicalDeviceData.MemoryProperties.memoryTypeCount; i++)
        {
            if ((memRequirements.memoryTypeBits & 1) == 1)
            {
                if ((physicalDeviceData.MemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                {
					memoryTypeIndex = i;
					break;
                }
            }
        }

        memAlloc.memoryTypeIndex = memoryTypeIndex;
        assert(vkAllocateMemory(deviceData.device, &memAlloc, nullptr, &depthStencilData.Memory) == VK_SUCCESS);
        assert(vkBindImageMemory(deviceData.device, depthStencilData.Image, depthStencilData.Memory, 0) == VK_SUCCESS);

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthStencilData.Format;
        viewInfo.image = depthStencilData.Image;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

        if (depthStencilData.Format > VK_FORMAT_D16_UNORM_S8_UINT)
        {
			viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        assert(vkCreateImageView(deviceData.device, &viewInfo, nullptr, &depthStencilData.View) == VK_SUCCESS);
    }

    void VulkanGraphics::CreateRenderPass()
    {

        std::array<VkAttachmentDescription, 2> attachments = {};
        // Color attachment
        attachments[0].format = swapchainData.colorFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // Depth attachment
        attachments[1].format = depthStencilData.Format;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorReference = {};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthReference = {};
        depthReference.attachment = 1;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;
        subpassDescription.pDepthStencilAttachment = &depthReference;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;
        subpassDescription.pResolveAttachments = nullptr;

        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dependencies[0].dependencyFlags = 0;

        dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].dstSubpass = 0;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = 0;
        dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dependencies[1].dependencyFlags = 0;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        assert(vkCreateRenderPass(deviceData.device, &renderPassInfo, nullptr, &renderPass) == VK_SUCCESS);
    }

    VkCommandBuffer VulkanGraphics::BeginSingleTimeCommands()
    {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer;
		assert(vkAllocateCommandBuffers(deviceData.device, &allocInfo, &commandBuffer) == VK_SUCCESS);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    VkResult VulkanGraphics::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
        assert(vkCreateBuffer(deviceData.device, &bufferInfo, nullptr, &buffer) == VK_SUCCESS);

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(deviceData.device, buffer, &memRequirements);
        VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        assert(vkAllocateMemory(deviceData.device, &allocInfo, nullptr, &bufferMemory) == VK_SUCCESS);

        return VkResult::VK_SUCCESS;
    }

    VkResult VulkanGraphics::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        assert(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);

        VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

        VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = 0;
		VkFence fence;
		assert(vkCreateFence(deviceData.device, &fenceInfo, nullptr, &fence) == VK_SUCCESS);
        assert(vkQueueSubmit(deviceData.queue, 1, &submitInfo, fence) == VK_SUCCESS);
        assert(vkWaitForFences(deviceData.device, 1, &fence, VK_TRUE, 100000000000) == VK_SUCCESS);

        vkDestroyFence(deviceData.device, fence, nullptr);

		vkFreeCommandBuffers(deviceData.device, commandPool, 1, &commandBuffer);

        return VkResult::VK_SUCCESS;
    }

    uint32_t VulkanGraphics::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        for (uint32_t i = 0; i < physicalDeviceData.MemoryProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & 1) == 1)
            {
                if ((physicalDeviceData.MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
					return i;
                }
            }
            typeFilter >>= 1;
        }
        throw std::runtime_error("failed to find suitable memory type!");
        return 0;
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
