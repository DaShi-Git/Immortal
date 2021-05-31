#include "impch.h"
#include "VulkanImage.h"

namespace Immortal
{
namespace Vulkan
{
	inline VkImageType ImageType(VkExtent3D extent)
	{
		VkImageType result{};

		uint32_t dimension{ 0 };

		if (extent.width >= 1)
		{
			dimension++;
		}

		if (extent.height >= 1)
		{
			dimension++;
		}

		if (extent.depth > 1)
		{
			dimension++;
		}

		switch (dimension)
		{
			case 1:
				result = VK_IMAGE_TYPE_1D;
				break;
			case 2:
				result = VK_IMAGE_TYPE_2D;
				break;
			case 3:
				result = VK_IMAGE_TYPE_3D;
				break;
			default:
				IM_CORE_ERROR(LOGB("图片格式错误", "No image type found."));
				break;
		}

		return result;
	}
}
	

	VulkanImage::VulkanImage(VulkanDevice &device, VkImage handle, const VkExtent3D &extent, VkFormat format, VkImageUsageFlags imageUsage, VkSampleCountFlagBits sampleCount) :
		mDevice{ device },
		mHandle{ handle },
		mType{ Vulkan::ImageType(extent) },
		mExtent{ extent },
		mFormat{ format },
		mSampleCount{ sampleCount },
		mUsage{ imageUsage }
	{
		mSubresource.mipLevel   = 1;
		mSubresource.arrayLayer = 1;
	}

	VulkanImage::VulkanImage(VulkanDevice &device, const VkExtent3D &extent, VkFormat format, VkImageUsageFlags imageUsage, VmaMemoryUsage memoryUsage, VkSampleCountFlagBits sampleCount, UINT32 mipLevels, UINT32 arrayLayers, VkImageTiling tiling, VkImageCreateFlags flags, UINT32 numQueueFamilies, const UINT32 * queueFamilies) :
		mDevice{ device },
		mType{ Vulkan::ImageType(extent) },
		mExtent{ extent },
		mFormat{ format },
		mSampleCount{ sampleCount },
		mUsage{ imageUsage },
		mArrayLayerCount{ arrayLayers },
		mTiling{ tiling }
	{
		IM_CORE_ASSERT(mipLevels > 0, LOGB("Mip贴图应该大于一个层级", "Image should have at least one level"));
		IM_CORE_ASSERT(arrayLayers > 0, LOGB("图片至少有一层", "Image should have at least one level"));

		mSubresource.mipLevel = mipLevels;
		mSubresource.arrayLayer = arrayLayers;

		VkImageCreateInfo imageInfo{};
		imageInfo.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType   = mType;
		imageInfo.format      = format;
		imageInfo.extent      = extent;
		imageInfo.mipLevels   = mipLevels;
		imageInfo.arrayLayers = arrayLayers;
		imageInfo.samples     = sampleCount;
		imageInfo.tiling      = tiling;
		imageInfo.usage       = imageUsage;
		
		if (numQueueFamilies != 0)
		{
			imageInfo.sharingMode           = VK_SHARING_MODE_CONCURRENT;
			imageInfo.queueFamilyIndexCount = numQueueFamilies;
			imageInfo.pQueueFamilyIndices   = queueFamilies;
		}

		VmaAllocationCreateInfo memoryInfo{};
		memoryInfo.usage = memoryUsage;
		if (imageUsage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
		{
			memoryInfo.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		}

		Vulkan::Check(vmaCreateImage(device.MemoryAllocator(), &imageInfo, &memoryInfo, &mHandle, &mMemory, nullptr));
	}

}