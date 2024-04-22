#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <iostream>

#include "../Platform/Window.h"

namespace vvoxelen{
	class VulkanRender
	{
	public:
		VulkanRender(Window* currentWindow);
		~VulkanRender();

		void createInstance();
		void createSurface();
	private:
		Window* vvoxWindow;
		VkInstance instance;
		VkSurfaceKHR surface;
	};
}
