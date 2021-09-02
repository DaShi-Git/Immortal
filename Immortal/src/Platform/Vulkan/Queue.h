#pragma once

#include "vkcommon.h"

namespace Immortal
{
namespace Vulkan
{
	class Device;
	class Queue
	{
	public:
		using FamilyIndex = UINT32;
	public:
		enum class Type
		{
			Graphics      = VK_QUEUE_GRAPHICS_BIT,
			Compute       = VK_QUEUE_COMPUTE_BIT,
			Transfer      = VK_QUEUE_TRANSFER_BIT,
			SparseBinding = VK_QUEUE_SPARSE_BINDING_BIT,
			Protected     = VK_QUEUE_PROTECTED_BIT,
#ifdef VK_ENABLE_BETA_EXTENSIONS
			VideoDeCode   = VK_QUEUE_VIDEO_DECODE_BIT_KHR,
			VideoEncode   = VK_QUEUE_VIDEO_ENCODE_BIT_KHR,
#endif
			None
		};

	public:
		Queue(Device &device, UINT32 familyIndex, VkQueueFamilyProperties properties, VkBool32 canPresent, UINT32 index);

		Queue(const Queue &) = default;

		Queue(Queue &&other);

		~Queue();

	public:
		template <class T>
		T &Get()
		{
			if constexpr (std::is_same_v<T, FamilyIndex>)
			{
				return familyIndex;
			}
		}

		const VkQueue &Handle() const
		{
			return handle;
		}

		VkQueueFamilyProperties Properties() const
		{
			return properties;
		}

		VkBool32 IsPresentSupported() const
		{
			return presented;
		}

	private:
		Device &device;

		VkQueue handle{ VK_NULL_HANDLE };

		UINT32 familyIndex{ 0 };

		UINT32 index{ 0 };

		VkBool32 presented{ VK_FALSE };

		VkQueueFamilyProperties properties{};

	private:
		//Queue &operator=(const Queue &) = delete;
		Queue &operator=(Queue &&) = delete;
	};
}
}

