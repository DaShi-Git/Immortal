#pragma once

#include "Immortal/Render/RenderContext.h"

#include "VulkanDevice.h"
#include "VulkanInstance.h"

namespace Immortal
{
	class VulkanRenderContext : public RenderContext
	{
	public:
		VulkanRenderContext() = default;
		VulkanRenderContext(RenderContext::Description &desc);
		~VulkanRenderContext();

		virtual void Init() override;
		virtual void SwapBuffers() override;

	private:
		void *mHandle;
		Unique<VulkanInstance> mInstance;
		VulkanDevice   mDevice;
		VkSurfaceKHR   mSurface;

	public:
		static std::unordered_map<const char *, bool> InstanceExtensions;
	};
}

