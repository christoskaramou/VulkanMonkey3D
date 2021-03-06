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
#include "Framebuffer.h"
#include "RenderPass.h"
#include "RenderApi.h"

namespace pe
{
	Framebuffer::Framebuffer()
	{
		handle = make_ref(vk::Framebuffer());
	}
	
	void Framebuffer::Create(uint32_t width, uint32_t height, const vk::ImageView& view, const RenderPass& renderPass)
	{
		Create(width, height, std::vector<vk::ImageView> {view}, renderPass);
	}
	
	void Framebuffer::Create(
			uint32_t width, uint32_t height, const std::vector<vk::ImageView>& views, const RenderPass& renderPass
	)
	{
		
		vk::FramebufferCreateInfo fbci;
		fbci.renderPass = *renderPass.handle;
		fbci.attachmentCount = static_cast<uint32_t>(views.size());
		fbci.pAttachments = views.data();
		fbci.width = width;
		fbci.height = height;
		fbci.layers = 1;
		
		handle = make_ref(VulkanContext::Get()->device->createFramebuffer(fbci));
	}
	
	void Framebuffer::Destroy()
	{
		if (*handle)
		{
			VulkanContext::Get()->device->destroyFramebuffer(*handle);
			*handle = nullptr;
		}
	}
}
