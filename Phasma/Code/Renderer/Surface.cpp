/*
Copyright (c) 2018-2021 Christos Karamoustos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "PhasmaPch.h"
#include "Surface.h"
#include "../ECS/Context.h"
#include "Renderer.h"
#include "RenderApi.h"

namespace pe
{
	Surface::Surface()
	{
		surface = make_ref(vk::SurfaceKHR());
		actualExtent = make_ref(vk::Extent2D());
		capabilities = make_ref(vk::SurfaceCapabilitiesKHR());
		formatKHR = make_ref(vk::SurfaceFormatKHR());
		presentModeKHR = make_ref(vk::PresentModeKHR::eFifo);
	}
	
	Surface::~Surface()
	{
	}
	
	void Surface::Create(Context* ctx)
	{
		VkSurfaceKHR _vkSurface;
		if (!SDL_Vulkan_CreateSurface(
				ctx->GetSystem<Renderer>()->GetWindow(), VkInstance(*ctx->GetVKContext()->instance), &_vkSurface
		))
			throw std::runtime_error(SDL_GetError());
		
		int width, height;
		SDL_GL_GetDrawableSize(ctx->GetSystem<Renderer>()->GetWindow(), &width, &height);
		actualExtent = make_ref(vk::Extent2D {static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
		surface = make_ref(vk::SurfaceKHR(_vkSurface));
	}
	
	void Surface::FindCapabilities(Context* ctx)
	{
		auto gpu = ctx->GetVKContext()->gpu;
		auto caps = gpu->getSurfaceCapabilitiesKHR(*surface);
		// Ensure eTransferSrc bit for blit operations
		if (!(caps.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc))
			throw std::runtime_error("Surface doesnt support vk::ImageUsageFlagBits::eTransferSrc");
		capabilities = make_ref(caps);
	}
	
	void Surface::FindFormat(Context* ctx)
	{
		auto gpu = ctx->GetVKContext()->gpu;
		std::vector<vk::SurfaceFormatKHR> formats = gpu->getSurfaceFormatsKHR(*surface);
		auto format = formats[0];
		for (const auto& f : formats)
		{
			if (f.format == vk::Format::eB8G8R8A8Unorm && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				format = f;
		}
		
		// Check for blit operation
		auto const fProps = gpu->getFormatProperties(format.format);
		if (!(fProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc))
			throw std::runtime_error("No blit source operation supported");
		if (!(fProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst))
			throw std::runtime_error("No blit destination operation supported");
		
		formatKHR = make_ref(format);
	}
	
	void Surface::FindPresentationMode(Context* ctx)
	{
		auto gpu = ctx->GetVKContext()->gpu;
		std::vector<vk::PresentModeKHR> presentModes = gpu->getSurfacePresentModesKHR(*surface);
		
		for (const auto& i : presentModes)
			if (i == vk::PresentModeKHR::eMailbox)
			{
				presentModeKHR = make_ref(i);
				return;
			}
		
		for (const auto& i : presentModes)
			if (i == vk::PresentModeKHR::eImmediate)
			{
				presentModeKHR = make_ref(i);
				return;
			}
		
		presentModeKHR = make_ref(vk::PresentModeKHR::eFifo);
	}
	
	
	void Surface::FindProperties(Context* ctx)
	{
		FindCapabilities(ctx);
		FindFormat(ctx);
		FindPresentationMode(ctx);
	}
}
