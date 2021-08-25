#include "impch.h"
#include "Device.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "vkcommon.h"
#include "PhysicalDevice.h"

namespace Immortal
{
namespace Vulkan
{
	Device::Device(PhysicalDevice& physicalDevice, VkSurfaceKHR surface, std::unordered_map<const char*, bool> requestedExtensions)
		: physicalDevice(physicalDevice),
		  surface(surface)
	{
		LOG::INFO("Selected GPU: {0}", physicalDevice.Properties.deviceName);

		UINT32 propsCount = U32(physicalDevice.QueueFamilyProperties.size());
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(propsCount, { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO });
		std::vector<std::vector<float>>      queueProps(propsCount);

		for (UINT32 index = 0; index < propsCount; index++)
		{
			const VkQueueFamilyProperties& prop = physicalDevice.QueueFamilyProperties[index];
			if (physicalDevice.HighPriorityGraphicsQueue)
			{
				UINT32 graphicsQueueFamily = QueueFailyIndex(VK_QUEUE_GRAPHICS_BIT);
				if (graphicsQueueFamily == index)
				{
					queueProps[index].reserve(prop.queueCount);
					queueProps[index].push_back(1.0f);
					for (UINT32 i = 1; i < prop.queueCount; i++)
					{
						queueProps[index].push_back(0.5f);
					}
				}
			}
			else
			{
				queueProps[index].resize(prop.queueCount, 0.5f);
			}
			queueCreateInfos[index].pNext            = nullptr;
			queueCreateInfos[index].queueFamilyIndex = index;
			queueCreateInfos[index].queueCount       = prop.queueCount;
			queueCreateInfos[index].pQueuePriorities = queueProps[index].data();
		}

		// Check extensions to enable Vma dedicated Allocation
		UINT32 deviceExtensionCount;
		Vulkan::Check(vkEnumerateDeviceExtensionProperties(physicalDevice.Handle(), nullptr, &deviceExtensionCount, nullptr));

		mDeviceExtensions.resize(deviceExtensionCount);
		Vulkan::Check(vkEnumerateDeviceExtensionProperties(physicalDevice.Handle(), nullptr, &deviceExtensionCount, mDeviceExtensions.data()));

#if     IMMORTAL_CHECK_DEBUG
		if (!mDeviceExtensions.empty())
		{
			LOG::INFO("Device supports the following extensions: ");
			for (auto& ext : mDeviceExtensions)
			{
				LOG::INFO("  \t{0}", ext.extensionName);
			}
		}
#endif

		// @required
		CheckExtensionSupported();

		// @required
		// Check that extensions are supported before creating the device
		std::vector<const char*> unsupportedExtensions{};
		for (auto& ext : requestedExtensions)
		{
			if (IsExtensionSupport(ext.first))
			{
				enabledExtensions.emplace_back(ext.first);
			}
			else
			{
				unsupportedExtensions.emplace_back(ext.first);
			}
		}

		if (!enabledExtensions.empty())
		{
			LOG::INFO("Device supports the following requested extensions:");
			for (auto& ext : enabledExtensions)
			{
				LOG::INFO("  \t{0}", ext);
			}
		}

		if (!unsupportedExtensions.empty())
		{
			for (auto& ext : unsupportedExtensions)
			{
				auto isOptional = requestedExtensions[ext];
				if (isOptional)
				{
					LOG::WARN("Optional device extension {0} not available. Some features may be disabled", ext);
				}
				else
				{
					LOG::ERR("Required device extension {0} not available. Stop running!", ext);
					Vulkan::Check(VK_ERROR_EXTENSION_NOT_PRESENT);
				}
			}
		}

		// @required
		// @info flags is reserved for future use.
		// @warn enableLayer related is deprecated and ignored
		VkDeviceCreateInfo createInfo{};
		createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext                   = physicalDevice.LastRequestedExtensionFeature;
		createInfo.queueCreateInfoCount    = U32(queueCreateInfos.size());
		createInfo.pQueueCreateInfos       = queueCreateInfos.data();
		createInfo.enabledExtensionCount   = U32(enabledExtensions.size());
		createInfo.ppEnabledExtensionNames = enabledExtensions.data();
		createInfo.pEnabledFeatures        = &physicalDevice.RequestedFeatures;

		Check(vkCreateDevice(physicalDevice.Handle(), &createInfo, nullptr, &handle));

		queues.resize(propsCount);
		for (UINT32 queueFamilyIndex = 0U; queueFamilyIndex < propsCount; queueFamilyIndex++)
		{
			const auto& queueFamilyProps = physicalDevice.QueueFamilyProperties[queueFamilyIndex];
			VkBool32 presentSupported = physicalDevice.IsPresentSupported(surface, queueFamilyIndex);

			for (UINT32 queueIndex = 0U; queueIndex < queueFamilyProps.queueCount; queueIndex++)
			{
				queues[queueFamilyIndex].emplace_back(*this, queueFamilyIndex, queueFamilyProps, presentSupported, queueIndex);
			}
		}

		// @required
		VmaVulkanFunctions vmaVulkanFunc{};
		vmaVulkanFunc.vkAllocateMemory                    = vkAllocateMemory;
		vmaVulkanFunc.vkBindBufferMemory                  = vkBindBufferMemory;
		vmaVulkanFunc.vkBindImageMemory                   = vkBindImageMemory;
		vmaVulkanFunc.vkCreateBuffer                      = vkCreateBuffer;
		vmaVulkanFunc.vkCreateImage                       = vkCreateImage;
		vmaVulkanFunc.vkDestroyBuffer                     = vkDestroyBuffer;
		vmaVulkanFunc.vkDestroyImage                      = vkDestroyImage;
		vmaVulkanFunc.vkFlushMappedMemoryRanges           = vkFlushMappedMemoryRanges;
		vmaVulkanFunc.vkFreeMemory                        = vkFreeMemory;
		vmaVulkanFunc.vkGetBufferMemoryRequirements       = vkGetBufferMemoryRequirements;
		vmaVulkanFunc.vkGetImageMemoryRequirements        = vkGetImageMemoryRequirements;
		vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
		vmaVulkanFunc.vkGetPhysicalDeviceProperties       = vkGetPhysicalDeviceProperties;
		vmaVulkanFunc.vkInvalidateMappedMemoryRanges      = vkInvalidateMappedMemoryRanges;
		vmaVulkanFunc.vkMapMemory                         = vkMapMemory;
		vmaVulkanFunc.vkUnmapMemory                       = vkUnmapMemory;
		vmaVulkanFunc.vkCmdCopyBuffer                     = vkCmdCopyBuffer;

		// @required
		VmaAllocatorCreateInfo allocatorInfo{};
		allocatorInfo.physicalDevice = physicalDevice.Handle();
		allocatorInfo.device         = handle;
		allocatorInfo.instance       = physicalDevice.Get<Instance>().Handle();

		if (mHasBufferDeviceAddressName)
		{
			allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		}

		if (mHasGetMemoryRequirements && mHasDedicatedAllocation)
		{
			allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
			vmaVulkanFunc.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
			vmaVulkanFunc.vkGetImageMemoryRequirements2KHR  = vkGetImageMemoryRequirements2KHR;
		}

		allocatorInfo.pVulkanFunctions = &vmaVulkanFunc;
		Check(vmaCreateAllocator(&allocatorInfo, &mMemoryAllocator));

		// @required Command Pool
		commandPool = MakeUnique<CommandPool>(*this, FindQueueByFlags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0).Get<Queue::FamilyIndex>());
		// @required Fence Pool
		fencePool = MakeUnique<FencePool>(*this);
	}

	UINT32 Device::QueueFailyIndex(VkQueueFlagBits queueFlag)
	{
		const auto& queueFamilyProperties = physicalDevice.QueueFamilyProperties;

		// @required Compute Queue
		if (queueFlag & VK_QUEUE_COMPUTE_BIT)
		{
			for (UINT32 i = 0; i < U32(queueFamilyProperties.size()); i++)
			{
				if ((queueFamilyProperties[i].queueFlags & queueFlag) && !(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				{
					return i;
					break;
				}
			}
		}

		// @required Transfer Queue
		if (queueFlag & VK_QUEUE_TRANSFER_BIT)
		{
			for (UINT32 i = 0; i < U32(queueFamilyProperties.size()); i++)
			{
				if ((queueFamilyProperties[i].queueFlags & queueFlag) &&
				   !(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
				   !(queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
				{
					return i;
					break;
				}
			}
		}

		for (UINT32 i = 0; i < U32(queueFamilyProperties.size()); i++)
		{
			if (queueFamilyProperties[i].queueFlags & queueFlag)
			{
				return i;
				break;
			}
		}

		LOG::ERR("Counld not find a matching queue family index");
		return 0;
	}

	Device::~Device()
	{
		static auto DestroyVmaAllocator = [](VmaAllocator memoryAllocator)
		{
			VmaStats stats;
			vmaCalculateStats(memoryAllocator, &stats);
			LOG::INFO("Total device memory leaked: {0} bytes.", stats.total.usedBytes);

			vmaDestroyAllocator(memoryAllocator);
		};

		Vulkan::IfNotNullThen<VmaAllocator, DestroyVmaAllocator>(mMemoryAllocator);
		Vulkan::IfNotNullThen(vkDestroyDevice, handle);
	}

	bool Device::IsExtensionSupport(const char* extension) NOEXCEPT
	{
		return std::find_if(mDeviceExtensions.begin(), mDeviceExtensions.end(), [extension](auto& deviceExtension)
			{
				return Vulkan::Equals(deviceExtension.extensionName, extension);
			}) != mDeviceExtensions.end();
	}

	bool Device::IsEnabled(const char* extension) const NOEXCEPT
	{
		return std::find_if(enabledExtensions.begin(), enabledExtensions.end(), [extension](const char* enabledExtension)
			{
				return Vulkan::Equals(extension, enabledExtension);
			}) != enabledExtensions.end();
	}

	void Device::CheckExtensionSupported() NOEXCEPT
	{
		bool hasPerformanceQuery = false;
		bool hasHostQueryReset = false;

		for (auto& e : mDeviceExtensions)
		{
			if (Equals(e.extensionName, "VK_KHR_get_memory_requirements2"))
			{
				mHasGetMemoryRequirements = true;
			}
			if (Equals(e.extensionName, "VK_KHR_dedicated_allocation"))
			{
				mHasDedicatedAllocation = true;
			}
			if (Equals(e.extensionName, "VK_KHR_performance_query"))
			{
				hasPerformanceQuery = true;
			}
			if (Equals(e.extensionName, "VK_EXT_host_query_reset"))
			{
				hasHostQueryReset = true;
			}
			if (Equals(e.extensionName, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) && IsEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
			{
				mHasBufferDeviceAddressName = true;
			}
		}

		if (mHasGetMemoryRequirements && mHasDedicatedAllocation)
		{
			enabledExtensions.emplace_back("VK_KHR_get_memory_requirements2");
			enabledExtensions.emplace_back("VK_KHR_dedicated_allocation");

			LOG::INFO("Dedicated Allocation enabled");
		}

		if (hasPerformanceQuery && hasHostQueryReset)
		{
			auto& perfCounterFeatures       = physicalDevice.RequestExtensionFeatures<VkPhysicalDevicePerformanceQueryFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR);
			auto& host_query_reset_features = physicalDevice.RequestExtensionFeatures<VkPhysicalDeviceHostQueryResetFeatures>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES);
			LOG::INFO("Performance query enabled");
		}
	}

	const Queue& Device::SuitableGraphicsQueue()
	{
		for (UINT32 familyIndex = 0U; familyIndex < queues.size(); familyIndex++)
		{
			Queue& firstQueue = queues[familyIndex][0];

			UINT32 queueCount = firstQueue.Properties().queueCount;

			if (firstQueue.IsPresentSupported() && 0 < queueCount)
			{
				return queues[familyIndex][0];
			}
		}

		return FindQueueByFlags(VK_QUEUE_GRAPHICS_BIT, 0);
	}


	Queue& Device::FindQueueByFlags(VkQueueFlags flags, UINT32 queueIndex)
	{
		for (uint32_t familyIndex = 0U; familyIndex < queues.size(); familyIndex++)
		{
			Queue& firstQueue = queues[familyIndex][0];

			VkQueueFlags queueFlags = firstQueue.Properties().queueFlags;
			uint32_t     queueCount = firstQueue.Properties().queueCount;

			if (((queueFlags & flags) == flags) && queueIndex < queueCount)
			{
				return queues[familyIndex][queueIndex];
			}
		}

		LOG::ERR("Queue not found");
		return Utils::NullValue<Queue>();
	}
}
}
