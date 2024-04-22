
#define VK_USE_PLATFORM_WIN32_KHR

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include "Platform/Window.h"
#include "Graphics/VulkanGraphics.h"

#include <iostream>
#include <vector>

int main()
{
    vvoxelen::Window window("Voxel Engine", 800, 600);
    vvoxelen::VulkanGraphics render;
    render.createInstance();
    render.createSurface();
    window.run();
    return 0;
}
