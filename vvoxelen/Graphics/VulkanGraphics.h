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
		void createSurface();
		static VulkanGraphics* GetSingleton();
	private:
		VkInstance instance;
		VkSurfaceKHR surface;
		static VulkanGraphics* singleton;
		std::vector<const char*> GetRequiredExtensions();
	};
}
