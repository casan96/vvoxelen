#pragma once


#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <vector>
namespace vvoxelen {
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
	};
	class Cube
	{
	private:
		std::vector<Vertex> vertices = {
				{ { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f } },
				{ {  1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f } },
				{ {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f } },
				{ { -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f } },

				{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f } },
				{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f } },
				{ {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f } },
				{ {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f } },

				{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f } },
				{ {  1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f } },
				{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f } },
				{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f } },

				{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f } },
				{ { -1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f } },
				{ { -1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f } },
				{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f } },

				{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f } },
				{ { -1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f } },
				{ { -1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f } },
				{ {  1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f } },

				{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f } },
				{ {  1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f } },
				{ {  1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f } },
				{ { -1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f } },
		};
		std::vector<uint32_t> indices = {
			0,1,2, 0,2,3, 4,5,6,  4,6,7, 8,9,10, 8,10,11, 12,13,14, 12,14,15, 16,17,18, 16,18,19, 20,21,22, 20,22,23
		};

		VkBuffer vertexBuffer{};
		VkDeviceMemory vertexBufferMemory{};

		VkBuffer indexBuffer{};
		VkDeviceMemory indexBufferMemory{};

		VkImage textureImage{};
		VkDeviceMemory textureImageMemory{};

		VkSampler textureSampler{};
		VkImageView textureImageView{};

		void loadTexture();
	public:

		Cube();
		~Cube();

	};
};

