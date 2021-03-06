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

#pragma once

#include "Buffer.h"
#include "Image.h"
#include "Pipeline.h"
#include "../Core/Math.h"
#include "../Camera/Camera.h"
#include "RenderPass.h"
#include "Framebuffer.h"

namespace vk
{
	class DescriptorSet;
}

namespace pe
{
	struct ShadowsUBO
	{
		mat4 projection, view;
		float castShadows;
		float maxCascadeDist0;
		float maxCascadeDist1;
		float maxCascadeDist2;
	};
	
	class Shadows
	{
	public:
		Shadows();
		
		~Shadows();
		
		ShadowsUBO shadows_UBO[3] {};
		static uint32_t imageSize;
		RenderPass renderPass;
		std::vector<Image> textures {};
		Ref<std::vector<vk::DescriptorSet>> descriptorSets;
		std::vector<Framebuffer> framebuffers {};
		std::vector<Buffer> uniformBuffers {};
		Pipeline pipeline;
		
		void update(Camera& camera);
		
		void createUniformBuffers();
		
		void createDescriptorSets();
		
		void createRenderPass();
		
		void createFrameBuffers();
		
		void createPipeline();
		
		void destroy();
	};
}
