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
#include "SSAO.h"
#include <deque>
#include "../Renderer/Swapchain.h"
#include "../Renderer/Surface.h"
#include "../GUI/GUI.h"
#include "../Shader/Shader.h"
#include "../Core/Queue.h"
#include "../Renderer/RenderApi.h"

namespace pe
{
	SSAO::SSAO()
	{
		DSet = make_ref(vk::DescriptorSet());
		DSBlur = make_ref(vk::DescriptorSet());
	}
	
	SSAO::~SSAO()
	{
	}
	
	void SSAO::createUniforms(std::map<std::string, Image>& renderTargets)
	{
		// kernel buffer
		std::vector<vec4> kernel {};
		for (unsigned i = 0; i < 16; i++)
		{
			vec3 sample(rand(-1.f, 1.f), rand(-1.f, 1.f), rand(0.f, 1.f));
			sample = normalize(sample);
			sample *= rand(0.f, 1.f);
			float scale = float(i) / 16.f;
			scale = lerp(.1f, 1.f, scale * scale);
			kernel.emplace_back(sample * scale, 0.f);
		}
		UB_Kernel.CreateBuffer(sizeof(vec4) * 16, BufferUsage::UniformBuffer, MemoryProperty::HostVisible);
		UB_Kernel.Map();
		UB_Kernel.CopyData(kernel.data());
		UB_Kernel.Flush();
		UB_Kernel.Unmap();
		
		// noise image
		std::vector<vec4> noise {};
		for (unsigned int i = 0; i < 16; i++)
			noise.emplace_back(rand(-1.f, 1.f), rand(-1.f, 1.f), 0.f, 1.f);
		
		Buffer staging;
		const uint64_t bufSize = sizeof(vec4) * 16;
		staging.CreateBuffer(bufSize, BufferUsage::TransferSrc, MemoryProperty::HostVisible);
		staging.Map();
		staging.CopyData(noise.data());
		staging.Flush();
		staging.Unmap();
		
		noiseTex.filter = make_ref(vk::Filter::eNearest);
		noiseTex.minLod = 0.0f;
		noiseTex.maxLod = 0.0f;
		noiseTex.maxAnisotropy = 1.0f;
		noiseTex.format = make_ref(vk::Format::eR16G16B16A16Sfloat);
		noiseTex.createImage(
				4, 4, vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
				vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal
		);
		noiseTex.transitionImageLayout(vk::ImageLayout::ePreinitialized, vk::ImageLayout::eTransferDstOptimal);
		noiseTex.copyBufferToImage(*staging.GetBufferVK());
		noiseTex.transitionImageLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
		noiseTex.createImageView(vk::ImageAspectFlagBits::eColor);
		noiseTex.createSampler();
		staging.Destroy();
		// pvm uniform
		UB_PVM.CreateBuffer(3 * sizeof(mat4), BufferUsage::UniformBuffer, MemoryProperty::HostVisible);
		UB_PVM.Map();
		UB_PVM.Zero();
		UB_PVM.Flush();
		UB_PVM.Unmap();
		
		// DESCRIPTOR SET FOR SSAO
		const vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo {
				*VulkanContext::Get()->descriptorPool,    //DescriptorPool descriptorPool;
				1,                                                //uint32_t descriptorSetCount;
				&Pipeline::getDescriptorSetLayoutSSAO()            //const DescriptorSetLayout* pSetLayouts;
		};
		DSet = make_ref(VulkanContext::Get()->device->allocateDescriptorSets(allocInfo).at(0));
		
		// DESCRIPTOR SET FOR SSAO BLUR
		const vk::DescriptorSetAllocateInfo allocInfoBlur = vk::DescriptorSetAllocateInfo {
				*VulkanContext::Get()->descriptorPool,    //DescriptorPool descriptorPool;
				1,                                                //uint32_t descriptorSetCount;
				&Pipeline::getDescriptorSetLayoutSSAOBlur()    //const DescriptorSetLayout* pSetLayouts;
		};
		DSBlur = make_ref(VulkanContext::Get()->device->allocateDescriptorSets(allocInfoBlur).at(0));
		
		updateDescriptorSets(renderTargets);
	}
	
	void SSAO::updateDescriptorSets(std::map<std::string, Image>& renderTargets)
	{
		std::deque<vk::DescriptorImageInfo> dsii {};
		const auto wSetImage = [&dsii](
				const vk::DescriptorSet& dstSet, uint32_t dstBinding, Image& image,
				vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
		)
		{
			dsii.emplace_back(*image.sampler, *image.view, imageLayout);
			return vk::WriteDescriptorSet {
					dstSet, dstBinding, 0, 1, vk::DescriptorType::eCombinedImageSampler, &dsii.back(), nullptr, nullptr
			};
		};
		std::deque<vk::DescriptorBufferInfo> dsbi {};
		const auto wSetBuffer = [&dsbi](const vk::DescriptorSet& dstSet, uint32_t dstBinding, Buffer& buffer)
		{
			dsbi.emplace_back(*buffer.GetBufferVK(), 0, buffer.Size());
			return vk::WriteDescriptorSet {
					dstSet, dstBinding, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &dsbi.back(), nullptr
			};
		};
		
		std::vector<vk::WriteDescriptorSet> writeDescriptorSets {
				wSetImage(*DSet, 0, renderTargets["depth"]),
				wSetImage(*DSet, 1, renderTargets["normal"]),
				wSetImage(*DSet, 2, noiseTex),
				wSetBuffer(*DSet, 3, UB_Kernel),
				wSetBuffer(*DSet, 4, UB_PVM),
				wSetImage(*DSBlur, 0, renderTargets["ssao"])
		};
		VulkanContext::Get()->device->updateDescriptorSets(writeDescriptorSets, nullptr);
	}
	
	void SSAO::draw(vk::CommandBuffer cmd, uint32_t imageIndex, Image& image)
	{
		// SSAO image
		vk::ClearValue clearColor;
		memcpy(clearColor.color.float32, GUI::clearColor.data(), 4 * sizeof(float));
		
		
		std::vector<vk::ClearValue> clearValues = {clearColor};
		
		vk::RenderPassBeginInfo rpi;
		rpi.renderPass = *renderPass.handle;
		rpi.framebuffer = *framebuffers[imageIndex].handle;
		rpi.renderArea.offset = vk::Offset2D {0, 0};
		rpi.renderArea.extent = *image.extent;
		rpi.clearValueCount = 1;
		rpi.pClearValues = clearValues.data();
		
		image.changeLayout(cmd, LayoutState::ColorWrite);
		cmd.beginRenderPass(rpi, vk::SubpassContents::eInline);
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.handle);
		const vk::DescriptorSet descriptorSets = {*DSet};
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, descriptorSets, nullptr);
		cmd.draw(3, 1, 0, 0);
		cmd.endRenderPass();
		image.changeLayout(cmd, LayoutState::ColorRead);
		
		// new blurry SSAO image
		rpi.renderPass = *blurRenderPass.handle;
		rpi.framebuffer = *blurFramebuffers[imageIndex].handle;
		
		cmd.beginRenderPass(rpi, vk::SubpassContents::eInline);
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineBlur.handle);
		const vk::DescriptorSet descriptorSetsBlur = {*DSBlur};
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineBlur.layout, 0, descriptorSetsBlur, nullptr);
		cmd.draw(3, 1, 0, 0);
		cmd.endRenderPass();
	}
	
	void SSAO::destroy()
	{
		UB_Kernel.Destroy();
		UB_PVM.Destroy();
		noiseTex.destroy();
		
		renderPass.Destroy();
		blurRenderPass.Destroy();
		
		for (auto& frameBuffer : framebuffers)
			frameBuffer.Destroy();
		for (auto& frameBuffer : blurFramebuffers)
			frameBuffer.Destroy();
		
		pipeline.destroy();
		pipelineBlur.destroy();
		if (Pipeline::getDescriptorSetLayoutSSAO())
		{
			VulkanContext::Get()->device->destroyDescriptorSetLayout(Pipeline::getDescriptorSetLayoutSSAO());
			Pipeline::getDescriptorSetLayoutSSAO() = nullptr;
		}
		if (Pipeline::getDescriptorSetLayoutSSAOBlur())
		{
			VulkanContext::Get()->device->destroyDescriptorSetLayout(Pipeline::getDescriptorSetLayoutSSAOBlur());
			Pipeline::getDescriptorSetLayoutSSAOBlur() = nullptr;
		}
	}
	
	void SSAO::update(Camera& camera)
	{
		if (GUI::show_ssao)
		{
			pvm[0] = camera.projection;
			pvm[1] = camera.view;
			pvm[2] = camera.invProjection;
			
			Queue::memcpyRequest(&UB_PVM, {{&pvm, sizeof(pvm), 0}});
			//UB_PVM.map();
			//memcpy(UB_PVM.data, pvm, sizeof(pvm));
			//UB_PVM.flush();
			//UB_PVM.unmap();
		}
	}
	
	void SSAO::createRenderPasses(std::map<std::string, Image>& renderTargets)
	{
		renderPass.Create(*renderTargets["ssao"].format, vk::Format::eUndefined);
		blurRenderPass.Create(*renderTargets["ssaoBlur"].format, vk::Format::eUndefined);
	}
	
	void SSAO::createFrameBuffers(std::map<std::string, Image>& renderTargets)
	{
		createSSAOFrameBuffers(renderTargets);
		createSSAOBlurFrameBuffers(renderTargets);
	}
	
	void SSAO::createSSAOFrameBuffers(std::map<std::string, Image>& renderTargets)
	{
		auto vulkan = VulkanContext::Get();
		framebuffers.resize(vulkan->swapchain.images.size());
		for (size_t i = 0; i < vulkan->swapchain.images.size(); ++i)
		{
			uint32_t width = renderTargets["ssao"].width;
			uint32_t height = renderTargets["ssao"].height;
			vk::ImageView view = *renderTargets["ssao"].view;
			framebuffers[i].Create(width, height, view, renderPass);
		}
	}
	
	void SSAO::createSSAOBlurFrameBuffers(std::map<std::string, Image>& renderTargets)
	{
		auto vulkan = VulkanContext::Get();
		blurFramebuffers.resize(vulkan->swapchain.images.size());
		for (size_t i = 0; i < vulkan->swapchain.images.size(); ++i)
		{
			uint32_t width = renderTargets["ssaoBlur"].width;
			uint32_t height = renderTargets["ssaoBlur"].height;
			vk::ImageView view = *renderTargets["ssaoBlur"].view;
			blurFramebuffers[i].Create(width, height, view, blurRenderPass);
		}
	}
	
	void SSAO::createPipelines(std::map<std::string, Image>& renderTargets)
	{
		createPipeline(renderTargets);
		createBlurPipeline(renderTargets);
	}
	
	void SSAO::createPipeline(std::map<std::string, Image>& renderTargets)
	{
		Shader vert {"Shaders/Common/quad.vert", ShaderType::Vertex, true};
		Shader frag {"Shaders/SSAO/ssao.frag", ShaderType::Fragment, true};
		
		pipeline.info.pVertShader = &vert;
		pipeline.info.pFragShader = &frag;
		pipeline.info.width = renderTargets["ssao"].width_f;
		pipeline.info.height = renderTargets["ssao"].height_f;
		pipeline.info.cullMode = CullMode::Back;
		pipeline.info.colorBlendAttachments = make_ref(
				std::vector<vk::PipelineColorBlendAttachmentState> {*renderTargets["ssao"].blentAttachment}
		);
		pipeline.info.descriptorSetLayouts = make_ref(
				std::vector<vk::DescriptorSetLayout> {Pipeline::getDescriptorSetLayoutSSAO()}
		);
		pipeline.info.renderPass = renderPass;
		
		pipeline.createGraphicsPipeline();
	}
	
	void SSAO::createBlurPipeline(std::map<std::string, Image>& renderTargets)
	{
		Shader vert {"Shaders/Common/quad.vert", ShaderType::Vertex, true};
		Shader frag {"Shaders/SSAO/ssaoBlur.frag", ShaderType::Fragment, true};
		
		pipelineBlur.info.pVertShader = &vert;
		pipelineBlur.info.pFragShader = &frag;
		pipelineBlur.info.width = renderTargets["ssaoBlur"].width_f;
		pipelineBlur.info.height = renderTargets["ssaoBlur"].height_f;
		pipelineBlur.info.cullMode = CullMode::Back;
		pipelineBlur.info.colorBlendAttachments = make_ref(
				std::vector<vk::PipelineColorBlendAttachmentState> {*renderTargets["ssaoBlur"].blentAttachment}
		);
		pipelineBlur.info.descriptorSetLayouts = make_ref(
				std::vector<vk::DescriptorSetLayout> {Pipeline::getDescriptorSetLayoutSSAOBlur()}
		);
		pipelineBlur.info.renderPass = blurRenderPass;
		
		pipelineBlur.createGraphicsPipeline();
	}
}
