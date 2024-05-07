#include "Cube.h"

#include "../Graphics/VulkanGraphics.h"
namespace vvoxelen{

	Cube::Cube()
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		VulkanGraphics::GetSingleton()->createBuffer(vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		void* mappedMemory{};
		vkMapMemory(VulkanGraphics::GetSingleton()->getDevice(), stagingBufferMemory, 0, vertices.size() * sizeof(Vertex), 0, &mappedMemory);
		memcpy(mappedMemory, vertices.data(), vertices.size() * sizeof(Vertex));

		vkUnmapMemory(VulkanGraphics::GetSingleton()->getDevice(), stagingBufferMemory);
		mappedMemory = nullptr;

		VkBuffer indexstagingBuffer;
		VkDeviceMemory indexstagingBufferMemory;
		
		VulkanGraphics::GetSingleton()->createBuffer(indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexstagingBuffer, indexstagingBufferMemory);
		vkMapMemory(VulkanGraphics::GetSingleton()->getDevice(), indexstagingBufferMemory, 0, indices.size() * sizeof(uint32_t), 0, &mappedMemory);
		memcpy(mappedMemory, indices.data(), indices.size() * sizeof(uint32_t));

		vkUnmapMemory(VulkanGraphics::GetSingleton()->getDevice(), indexstagingBufferMemory);
		mappedMemory = nullptr;

		VulkanGraphics::GetSingleton()->createBuffer(vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

		VulkanGraphics::GetSingleton()->createBuffer(indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		VulkanGraphics::GetSingleton()->copyBuffer(stagingBuffer, vertexBuffer, vertices.size() * sizeof(Vertex));
		VulkanGraphics::GetSingleton()->copyBuffer(indexstagingBuffer, indexBuffer, indices.size() * sizeof(uint32_t));

		vkDestroyBuffer(VulkanGraphics::GetSingleton()->getDevice(), stagingBuffer, nullptr);
		vkFreeMemory(VulkanGraphics::GetSingleton()->getDevice(), stagingBufferMemory, nullptr);
		
		vkDestroyBuffer(VulkanGraphics::GetSingleton()->getDevice(), indexstagingBuffer, nullptr);
		vkFreeMemory(VulkanGraphics::GetSingleton()->getDevice(), indexstagingBufferMemory, nullptr);
	}
	Cube::~Cube()
	{
		if (vertexBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(VulkanGraphics::GetSingleton()->getDevice(), vertexBuffer, nullptr);
		}
		if (vertexBufferMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(VulkanGraphics::GetSingleton()->getDevice(), vertexBufferMemory, nullptr);
		}
		if (indexBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(VulkanGraphics::GetSingleton()->getDevice(), indexBuffer, nullptr);
		}
		if (indexBufferMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(VulkanGraphics::GetSingleton()->getDevice(), indexBufferMemory, nullptr);
		}
	}
};
