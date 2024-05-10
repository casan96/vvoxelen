#include "Cube.h"

#include "../Graphics/VulkanGraphics.h"

#include <memory>
namespace vvoxelen{
	void Cube::loadTexture()
	{
		VkDeviceSize imageSize = sizeof(int32_t) * 16 * 16;
		//for the moment only a green color texture
		auto tex = std::make_unique<int32_t[]>(16*16);
		for (int i = 0; i < 16 * 16; i++)
		{
			tex[i] = 0x00ff00ff;
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateBuffer(VulkanGraphics::GetSingleton()->getDevice(), &bufferInfo, nullptr, &stagingBuffer);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memRequirements;

		vkGetBufferMemoryRequirements(VulkanGraphics::GetSingleton()->getDevice(), stagingBuffer, &memRequirements);

		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = VulkanGraphics::GetSingleton()->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkAllocateMemory(VulkanGraphics::GetSingleton()->getDevice(), &allocInfo, nullptr, &stagingBufferMemory);
		vkBindBufferMemory(VulkanGraphics::GetSingleton()->getDevice(), stagingBuffer, stagingBufferMemory, 0);

		void* mappedMemory{};
		vkMapMemory(VulkanGraphics::GetSingleton()->getDevice(), stagingBufferMemory, 0, imageSize, 0, &mappedMemory);
		memcpy(mappedMemory, tex.get(), imageSize);
		vkUnmapMemory(VulkanGraphics::GetSingleton()->getDevice(), stagingBufferMemory);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width = 16;
		region.imageExtent.height = 16;
		region.imageExtent.depth = 1;

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = 16;
		imageInfo.extent.height = 16;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;	
		imageInfo.format = VK_FORMAT_R8G8B8A8_SINT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateImage(VulkanGraphics::GetSingleton()->getDevice(), &imageInfo, nullptr, &textureImage);

		vkGetImageMemoryRequirements(VulkanGraphics::GetSingleton()->getDevice(), textureImage, &memRequirements);

		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = VulkanGraphics::GetSingleton()->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vkAllocateMemory(VulkanGraphics::GetSingleton()->getDevice(), &allocInfo, nullptr, &textureImageMemory);
		vkBindImageMemory(VulkanGraphics::GetSingleton()->getDevice(), textureImage, textureImageMemory, 0);

		VkCommandBuffer commandBuffer = VulkanGraphics::GetSingleton()->BeginSingleTimeCommands();

		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;

		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.image = textureImage;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		VkImageMemoryBarrier imageMemoryBarrier2{};
		imageMemoryBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier2.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier2.image = textureImage;
		imageMemoryBarrier2.subresourceRange = subresourceRange;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier2);

		VulkanGraphics::GetSingleton()->flushCmdBuffer(commandBuffer);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		vkCreateSampler(VulkanGraphics::GetSingleton()->getDevice(), &samplerInfo, nullptr, &textureSampler);

		VkImageViewCreateInfo imageviewInfo{};
		imageviewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageviewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageviewInfo.format = VK_FORMAT_R8G8B8A8_SINT;

		vkCreateImageView(VulkanGraphics::GetSingleton()->getDevice(), &imageviewInfo, nullptr, &textureImageView);

		vkFreeMemory(VulkanGraphics::GetSingleton()->getDevice(), stagingBufferMemory, nullptr);
		vkDestroyBuffer(VulkanGraphics::GetSingleton()->getDevice(), stagingBuffer, nullptr);


	}
	Cube::Cube()
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		VulkanGraphics::GetSingleton()->createBuffer(vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		vkBindBufferMemory(VulkanGraphics::GetSingleton()->getDevice(), stagingBuffer, stagingBufferMemory, 0);
		void* mappedMemory{};
		vkMapMemory(VulkanGraphics::GetSingleton()->getDevice(), stagingBufferMemory, 0, vertices.size() * sizeof(Vertex), 0, &mappedMemory);
		memcpy(mappedMemory, vertices.data(), vertices.size() * sizeof(Vertex));

		vkUnmapMemory(VulkanGraphics::GetSingleton()->getDevice(), stagingBufferMemory);
		mappedMemory = nullptr;

		VkBuffer indexstagingBuffer;
		VkDeviceMemory indexstagingBufferMemory;
		
		VulkanGraphics::GetSingleton()->createBuffer(indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexstagingBuffer, indexstagingBufferMemory);
		vkBindBufferMemory(VulkanGraphics::GetSingleton()->getDevice(), indexstagingBuffer, indexstagingBufferMemory, 0);
		vkMapMemory(VulkanGraphics::GetSingleton()->getDevice(), indexstagingBufferMemory, 0, indices.size() * sizeof(uint32_t), 0, &mappedMemory);
		memcpy(mappedMemory, indices.data(), indices.size() * sizeof(uint32_t));

		vkUnmapMemory(VulkanGraphics::GetSingleton()->getDevice(), indexstagingBufferMemory);
		mappedMemory = nullptr;

		VulkanGraphics::GetSingleton()->createBuffer(vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

		VulkanGraphics::GetSingleton()->createBuffer(indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		vkBindBufferMemory(VulkanGraphics::GetSingleton()->getDevice(), vertexBuffer, vertexBufferMemory, 0);
		vkBindBufferMemory(VulkanGraphics::GetSingleton()->getDevice(), indexBuffer, indexBufferMemory, 0);
		VulkanGraphics::GetSingleton()->copyBuffer(stagingBuffer, vertexBuffer, vertices.size() * sizeof(Vertex));
		VulkanGraphics::GetSingleton()->copyBuffer(indexstagingBuffer, indexBuffer, indices.size() * sizeof(uint32_t));

		vkDestroyBuffer(VulkanGraphics::GetSingleton()->getDevice(), stagingBuffer, nullptr);
		vkFreeMemory(VulkanGraphics::GetSingleton()->getDevice(), stagingBufferMemory, nullptr);
		
		vkDestroyBuffer(VulkanGraphics::GetSingleton()->getDevice(), indexstagingBuffer, nullptr);
		vkFreeMemory(VulkanGraphics::GetSingleton()->getDevice(), indexstagingBufferMemory, nullptr);
	}
	Cube::~Cube()
	{

		vkDestroyImage(VulkanGraphics::GetSingleton()->getDevice(), textureImage, nullptr);
		vkFreeMemory(VulkanGraphics::GetSingleton()->getDevice(), textureImageMemory, nullptr);
		vkDestroySampler(VulkanGraphics::GetSingleton()->getDevice(), textureSampler, nullptr);
		vkDestroyImageView(VulkanGraphics::GetSingleton()->getDevice(), textureImageView, nullptr);
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
