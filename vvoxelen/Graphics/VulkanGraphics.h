#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <iostream>
#include <assert.h>

#include "../Platform/Window.h"

namespace vvoxelen{
	class VulkanGraphics
	{
	public:
		VulkanGraphics();
		~VulkanGraphics();

		void initVulkan();
		void createInstance();
		void createPhysicalDevice();
		VkCommandPool createCommandPool(int32_t queueFamilyIndex);

		int32_t getGraphicsQueueFamilyIndex(VkQueueFlagBits queueFlags);

		static VulkanGraphics* GetSingleton();
	private:
		VkInstance instance = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		struct {
			VkPhysicalDevice device = VK_NULL_HANDLE;
			VkPhysicalDeviceProperties Properties{};
			VkPhysicalDeviceFeatures Features{};
			VkPhysicalDeviceMemoryProperties MemoryProperties{};
			std::vector<VkQueueFamilyProperties> QueueFamilyProperties;
		} physicalDeviceData;

		struct
		{
			VkDevice device = VK_NULL_HANDLE;
			VkQueue queue = VK_NULL_HANDLE;
			VkCommandPool commandPool = VK_NULL_HANDLE;
			struct
			{
				uint32_t graphics{};
				uint32_t compute{};
				uint32_t transfer{};
			} queueFamilyIndices;
			std::vector<const char*> deviceExtensions;
		} deviceData;

		std::vector<const char*> instanceExtensions;

		struct
		{
			VkSwapchainKHR swapchain = VK_NULL_HANDLE;
			VkFormat format;
			VkColorSpaceKHR colorSpace;
			uint32_t imageCount{};
			std::vector<VkImage> images;
			std::vector<VkImageView> imageViews;
			std::vector<VkFramebuffer> framebuffers;
		} swapchainData;

		struct 
		{
			VkSemaphore presentComplete = VK_NULL_HANDLE;
			VkSemaphore renderComplete = VK_NULL_HANDLE;
		} semaphoreData;
		
		VkSubmitInfo submitInfo{};
		VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		static VulkanGraphics* singleton;
	};
}
