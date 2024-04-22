
#include "Window.h"

namespace vvoxelen{

    Window* Window::singleton = nullptr;

	Window::Window(std::string title, int width, int height) : title(title), width(width), height(height)
	{
        SDL_assert(singleton == nullptr);
        singleton = this;
        // Create an SDL window that supports Vulkan rendering.
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            std::cout << "Could not initialize SDL." << std::endl;
            exit(1);
        }
        window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN);
        if (window == NULL) {
            std::cout << "Could not create SDL window." << std::endl;
            exit(1);
        }
	}
    Window::~Window()
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    // Get WSI extensions from SDL 
    std::vector<const char*> Window::GetSDLExtensions()
    {
        unsigned int extension_count;
        if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, NULL)) {
            std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
            exit(1);
        }
        std::vector<const char*> extensions(extension_count);
        if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data())) {
            std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
            exit(1);
        }
        return extensions;
    }
    bool Window::CreateVulkanSurface(VkInstance instance, VkSurfaceKHR& surface)
    {
        return SDL_Vulkan_CreateSurface(window, instance, &surface) == SDL_TRUE;
    }
    void Window::run()
    {
        // Poll for user input.
        bool stillRunning = true;
        while (stillRunning) {

            SDL_Event event;
            while (SDL_PollEvent(&event)) {

                switch (event.type) {

                case SDL_QUIT:
                    stillRunning = false;
                    break;

                default:
                    // Do nothing.
                    break;
                }
            }

            SDL_Delay(10);
        }
    }
    Window* Window::getSingleton()
    {
        return singleton;
    }
}
