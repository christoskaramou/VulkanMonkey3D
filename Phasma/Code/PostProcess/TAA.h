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

#include "../Renderer/Buffer.h"
#include "../Renderer/Pipeline.h"
#include "../Renderer/Image.h"
#include "../Core/Math.h"
#include "../Camera/Camera.h"
#include "../Renderer/RenderPass.h"
#include "../Renderer/Framebuffer.h"
#include <vector>
#include <map>

namespace vk
{
	class DescriptorSet;
	
	class CommandBuffer;
}

namespace pe
{
	class TAA
	{
	public:
		TAA();
		
		~TAA();
		
		std::vector<Framebuffer> framebuffers {}, framebuffersSharpen {};
		Pipeline pipeline, pipelineSharpen;
		RenderPass renderPass, renderPassSharpen;
		Ref<vk::DescriptorSet> DSet, DSetSharpen;
		Image previous;
		Image frameImage;
		
		struct UBO
		{
			vec4 values;
			vec4 sharpenValues;
			mat4 invProj;
		} ubo;
		Buffer uniform;
		
		void Init();
		
		void update(const Camera& camera);
		
		void createUniforms(std::map<std::string, Image>& renderTargets);
		
		void updateDescriptorSets(std::map<std::string, Image>& renderTargets);
		
		void draw(vk::CommandBuffer cmd, uint32_t imageIndex, std::map<std::string, Image>& renderTargets);
		
		void createRenderPasses(std::map<std::string, Image>& renderTargets);
		
		void createFrameBuffers(std::map<std::string, Image>& renderTargets);
		
		void createPipeline(std::map<std::string, Image>& renderTargets);
		
		void createPipelineSharpen(std::map<std::string, Image>& renderTargets);
		
		void createPipelines(std::map<std::string, Image>& renderTargets);
		
		void saveImage(const vk::CommandBuffer& cmd, Image& source) const;
		
		void destroy();
	};
}
