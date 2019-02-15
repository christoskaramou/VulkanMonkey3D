#include "Renderer.h"
#include "../Event/Event.h"

using namespace vm;

Renderer::Renderer(SDL_Window* window)
{
	ctx.vulkan.window = window;
	// INIT ALL VULKAN CONTEXT
	ctx.initVulkanContext();
	// INIT RENDERING
	ctx.initRendering();
	//LOAD RESOURCES
	ctx.loadResources();
	// CREATE UNIFORMS AND DESCRIPTOR SETS
	ctx.createUniforms();
	// init is done
	prepared = true;
}

Renderer::~Renderer()
{
	ctx.vulkan.device.waitIdle();
	if (Model::models.size() == 0) {
		if (Model::descriptorSetLayout) {
			ctx.vulkan.device.destroyDescriptorSetLayout(Model::descriptorSetLayout);
			Model::descriptorSetLayout = nullptr;
		}

		if (Mesh::descriptorSetLayout) {
			ctx.vulkan.device.destroyDescriptorSetLayout(Mesh::descriptorSetLayout);
			Mesh::descriptorSetLayout = nullptr;
		}
	}
	for (auto &model : Model::models)
		model.destroy();
	ctx.shadows.destroy();
	ctx.compute.destroy();
	ctx.deferred.destroy();
	ctx.ssao.destroy();
	ctx.ssr.destroy();
	ctx.motionBlur.destroy();
	ctx.skyBox.destroy();
	ctx.terrain.destroy();
	ctx.gui.destroy();
	ctx.lightUniforms.destroy();
	ctx.metrics.destroy();
	ctx.destroyVkContext();
}

void Renderer::checkQueue()
{
	// TODO: make an other command pool for multithreading
	for (auto& queue : Queue::loadModel) {
		VulkanContext::getVulkanContext().device.waitIdle();
		Model::models.push_back(Model());
		Model::models.back().loadModel(std::get<0>(queue), std::get<1>(queue)); // path, name
		GUI::modelList.push_back(std::get<1>(queue));
		for (auto& dll : Script::dlls) {
			std::string mName = Model::models.back().name.substr(0, Model::models.back().name.find_last_of("."));
			if (dll == mName) {
				Model::models.back().script = std::make_unique<Script>(dll.c_str());
				break;
			}
		}
		Queue::loadModel.pop_front();
	}
}

void Renderer::update(float delta)
{
	FIRE_EVENT(Event::OnUpdate);

	// check for commands in queue
	checkQueue();

	// universal scripts
	for (auto& s : ctx.scripts)
		s->update(delta);

	// update camera matrices
	ctx.camera_main.update();

	// MODELS
	for (auto &model : Model::models)
		model.update(ctx.camera_main, delta);

	// GUI
	ctx.gui.update();

	// LIGHTS
	ctx.lightUniforms.update(ctx.camera_main);

	// SSAO
	ctx.ssao.update(ctx.camera_main);

	// SSR
	ctx.ssr.update(ctx.camera_main);

	// MOTION BLUR
	ctx.motionBlur.update(ctx.camera_main);

	// SHADOWS
	ctx.shadows.update(ctx.camera_main);

	// SKYBOX
	ctx.skyBox.update(ctx.camera_main);

	//TERRAIN
	//if (ctx.terrain.render) {
	//	const mat4 pvm[3]{ projection, view };
	//	memcpy(ctx.terrain.uniformBuffer.data, &pvm, sizeof(pvm));
	//}
}

void Renderer::recordComputeCmds(const uint32_t sizeX, const uint32_t sizeY, const uint32_t sizeZ)
{
	auto beginInfo = vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		.setPInheritanceInfo(nullptr);
	ctx.vulkan.computeCmdBuffer.begin(beginInfo);

	ctx.vulkan.computeCmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, ctx.compute.pipeline.pipeline);
	ctx.vulkan.computeCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, ctx.compute.pipeline.compinfo.layout, 0, ctx.compute.DSCompute, nullptr);

	ctx.vulkan.computeCmdBuffer.dispatch(sizeX, sizeY, sizeZ);
	ctx.vulkan.computeCmdBuffer.end();
}

void Renderer::recordDeferredCmds(const uint32_t& imageIndex)
{
	vec2 surfSize(WIDTH_f, HEIGHT_f);
	vec2 winPos((float*)&GUI::winPos);
	vec2 winSize((float*)&GUI::winSize);

	vec2 UVOffset[2] = { winPos / surfSize, winSize / surfSize };

	ctx.camera_main.renderArea.update(winPos, winSize);

	auto beginInfo = vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		.setPInheritanceInfo(nullptr);

	auto& cmd = ctx.vulkan.dynamicCmdBuffer;
	cmd.begin(beginInfo);
	ctx.metrics.start(cmd);
	cmd.setViewport(0, ctx.camera_main.renderArea.viewport);
	cmd.setScissor(0, ctx.camera_main.renderArea.scissor);

	// SKYBOX
	ctx.skyBox.draw(imageIndex);

	if (Model::models.size() > 0) {
		// MODELS
		Model::batchStart(imageIndex, ctx.deferred);
		for (uint32_t m = 0; m < Model::models.size(); m++)
			Model::models[m].draw();
		Model::batchEnd();

		// SCREEN SPACE AMBIENT OCCLUSION
		if (GUI::show_ssao)
			ctx.ssao.draw(imageIndex, UVOffset);

		// SCREEN SPACE REFLECTIONS
		if (GUI::show_ssr)
			ctx.ssr.draw(imageIndex, UVOffset);

		// COMPOSITION
		ctx.deferred.draw(imageIndex, ctx.shadows, ctx.skyBox, ctx.camera_main.invViewProjection, UVOffset);
		
		// MOTION BLUR
		if (GUI::show_motionBlur)
			ctx.motionBlur.draw(imageIndex, UVOffset);
	}

	// GUI
	ctx.gui.draw(imageIndex);

	ctx.metrics.end();

	cmd.end();
}

void Renderer::recordShadowsCmds(const uint32_t& imageIndex)
{
	// Render Pass (shadows mapping) (outputs the depth image with the light POV)

	vk::DeviceSize offset = vk::DeviceSize();
	std::array<vk::ClearValue, 1> clearValuesShadows{};
	clearValuesShadows[0].depthStencil = { 0.0f, 0 };

	vk::CommandBufferBeginInfo beginInfoShadows;
	beginInfoShadows.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	vk::RenderPassBeginInfo renderPassInfoShadows;
	renderPassInfoShadows.renderPass = ctx.shadows.renderPass;
	renderPassInfoShadows.renderArea = { { 0, 0 },{ Shadows::imageSize, Shadows::imageSize } };
	renderPassInfoShadows.clearValueCount = static_cast<uint32_t>(clearValuesShadows.size());
	renderPassInfoShadows.pClearValues = clearValuesShadows.data();

	auto& cmd = ctx.vulkan.shadowCmdBuffer;
	cmd.begin(beginInfoShadows);
	cmd.setDepthBias(GUI::depthBias[0], GUI::depthBias[1], GUI::depthBias[2]);

	for (uint32_t i = 0; i < ctx.shadows.textures.size(); i++) {
		// depth[i] image ===========================================================
		renderPassInfoShadows.framebuffer = ctx.shadows.frameBuffers[i * ctx.vulkan.swapchain->images.size() + imageIndex]; // e.g. for 2 swapchain images - 1st(0,2,4) and 2nd(1,3,5)
		cmd.beginRenderPass(renderPassInfoShadows, vk::SubpassContents::eInline);
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ctx.shadows.pipeline.pipeline);
		for (uint32_t m = 0; m < Model::models.size(); m++) {
			if (Model::models[m].render) {
				cmd.bindVertexBuffers(0, Model::models[m].vertexBuffer.buffer, offset);
				cmd.bindIndexBuffer(Model::models[m].indexBuffer.buffer, 0, vk::IndexType::eUint32);
				cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, ctx.shadows.pipeline.pipeinfo.layout, 0, { ctx.shadows.descriptorSets[i], Model::models[m].descriptorSet }, nullptr);

				for (auto& mesh : Model::models[m].meshes) {
					if (mesh.render)
						cmd.drawIndexed(static_cast<uint32_t>(mesh.indices.size()), 1, mesh.indexOffset, mesh.vertexOffset, 0);
				}
			}
		}
		cmd.endRenderPass();
		// ==========================================================================
	}
	cmd.end();
}

void Renderer::present()
{
	if (!prepared) return;

	if (useCompute) {
		recordComputeCmds(2, 2, 1);
		auto const siCompute = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&ctx.vulkan.computeCmdBuffer);
		ctx.vulkan.computeQueue.submit(siCompute, ctx.vulkan.fences[1]);
		ctx.vulkan.device.waitForFences(ctx.vulkan.fences[1], VK_TRUE, UINT64_MAX);
		ctx.vulkan.device.resetFences(ctx.vulkan.fences[1]);
	}

	// what stage of a pipeline at a command buffer to wait for the semaphores to be done until keep going
	const vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	uint32_t imageIndex;

	// if using shadows use the semaphore[0], record and submit the shadow commands, else use the semaphore[1]
	if (GUI::shadow_cast) {
		imageIndex = ctx.vulkan.device.acquireNextImageKHR(ctx.vulkan.swapchain->swapchain, UINT64_MAX, ctx.vulkan.semaphores[0], vk::Fence()).value;

		recordShadowsCmds(imageIndex);

		auto const siShadows = vk::SubmitInfo()
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&ctx.vulkan.semaphores[0])
			.setPWaitDstStageMask(waitStages)
			.setCommandBufferCount(1)
			.setPCommandBuffers(&ctx.vulkan.shadowCmdBuffer)
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(&ctx.vulkan.semaphores[1]);
		ctx.vulkan.graphicsQueue.submit(siShadows, nullptr);
	}
	else
		imageIndex = ctx.vulkan.device.acquireNextImageKHR(ctx.vulkan.swapchain->swapchain, UINT64_MAX, ctx.vulkan.semaphores[1], vk::Fence()).value;

	recordDeferredCmds(imageIndex);

	// submit the command buffer
	auto const si = vk::SubmitInfo()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&ctx.vulkan.semaphores[1])
		.setPWaitDstStageMask(waitStages)
		.setCommandBufferCount(1)
		.setPCommandBuffers(&ctx.vulkan.dynamicCmdBuffer)
		.setSignalSemaphoreCount(1)
		.setPSignalSemaphores(&ctx.vulkan.semaphores[2]);
	ctx.vulkan.graphicsQueue.submit(si, ctx.vulkan.fences[0]);

	// Presentation
	auto const pi = vk::PresentInfoKHR()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&ctx.vulkan.semaphores[2])
		.setSwapchainCount(1)
		.setPSwapchains(&ctx.vulkan.swapchain->swapchain)
		.setPImageIndices(&imageIndex)
		.setPResults(nullptr); //optional
	ctx.vulkan.presentQueue.presentKHR(pi);

	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> duration = start - Timer::frameStart;
	Timer::noWaitDelta = duration.count();
	ctx.vulkan.device.waitForFences(ctx.vulkan.fences[0], VK_TRUE, UINT64_MAX);
	ctx.vulkan.device.resetFences(ctx.vulkan.fences[0]);

	if (overloadedGPU)
		ctx.vulkan.presentQueue.waitIdle(); // user set, when GPU can't catch the CPU commands 
	duration = std::chrono::high_resolution_clock::now() - start;
	waitingTime = duration.count();
}