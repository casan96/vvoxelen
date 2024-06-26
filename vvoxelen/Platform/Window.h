#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <string>
#include <iostream>
#include <vector>

namespace vvoxelen
{
	class Window
	{
	public:
		Window(std::string title = "vvoxelen", int width = 800, int height = 600);
		~Window();
		std::vector<const char*> GetSDLExtensions();
		bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR& surface);
		std::string getTitle() { return title; }
		int getWidth() { return width; }
		int getHeight() { return height; }
		void setWidth(int width) { this->width = width; }
		void setHeight(int height) { this->height = height; }
		void run();
		static Window* getSingleton();
	private:
		SDL_Window* window;
		std::string title;
		int width;
		int height;

		static Window* singleton;
	};
}

