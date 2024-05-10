#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <iostream>
#include <assert.h>
#include <array>

#include "../Platform/Window.h"

namespace vvoxelen{
	class VulkanGraphics
	{
	public:
		VulkanGraphics();
		~VulkanGraphics();

		void initVulkan();

		void initSwapchain();

		void createInstance();

		void createLogicalDevice();

		void createCommandPool();

		void CreateCommandBuffers();

		void CreateFences();

		void setupSwapchain();

		void SetupDepthStencil();

		void CreateRenderPass();

		void flushCmdBuffer(VkCommandBuffer commandBuffer);

		void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free);

		VkCommandBuffer BeginSingleTimeCommands();

		VkResult createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

		VkResult copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		VkCommandPool createCommandPool(int32_t queueFamilyIndex);

		int32_t getGraphicsQueueFamilyIndex(VkQueueFlagBits queueFlags);

		VkDevice getDevice() { return deviceData.device; }

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
			VkFormat colorFormat;
			VkColorSpaceKHR colorSpace;
			uint32_t imageCount{};
			uint32_t queueNodeIndex = UINT32_MAX;
			struct SwapChainImageBuffer
			{
				VkImage image;
				VkImageView view;
			};
			std::vector<SwapChainImageBuffer> framebuffers;
			std::vector<VkImage> images;
		} swapchainData;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		struct 
		{
			VkSemaphore presentComplete = VK_NULL_HANDLE;
			VkSemaphore renderComplete = VK_NULL_HANDLE;
		} semaphoreData;
		
		struct
		{
			VkImage Image = VK_NULL_HANDLE;
			VkImageView View = VK_NULL_HANDLE;
			VkFormat Format = VK_FORMAT_UNDEFINED;
			VkDeviceMemory Memory = VK_NULL_HANDLE;
		} depthStencilData;

		VkSubmitInfo submitInfo{};

		VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		std::vector<VkCommandBuffer> commandBuffers;

		std::vector<VkFence> fences;

		VkRenderPass renderPass = VK_NULL_HANDLE;

		VkPipelineCache pipelineCache = VK_NULL_HANDLE;

		std::vector<VkFramebuffer> framebuffers;

		static VulkanGraphics* singleton;
	};
}
