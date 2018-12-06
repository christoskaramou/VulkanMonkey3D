#include "../include/Light.h"

using namespace vm;

Light::Light() : 
	color(rand(0.f, 1.f), rand(0.0f, 1.f), rand(0.f, 1.f), rand(0.f, 1.f)),
	position(rand(-3.5f, 3.5f), rand(.7f, .7f), rand(-3.5f, 3.5f), 1.f),
	attenuation(1.05f, 1.f, 1.f, 1.f)
{ }

Light::Light(const vec4& color, const vec4& position, const vec4& attenuation) :
	color(color),
	position(position),
	attenuation(attenuation)
{ }

Light Light::sun()
{
	return Light(
		vec4(1.f, 1.f, 1.f, .5f),
		vec4(0.f, 300.f, 10.f, 1.f),
		vec4(0.f, 0.f, 1.f, 1.f)
	);
}

void LightUniforms::createLightUniforms()
{
	uniform.createBuffer(sizeof(LightsUBO), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	uniform.data = vulkan->device.mapMemory(uniform.memory, 0, uniform.size);
	LightsUBO lubo;
	memcpy(uniform.data, &lubo, uniform.size);

	auto const allocateInfo = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(vulkan->descriptorPool)
		.setDescriptorSetCount(1)
		.setPSetLayouts(&descriptorSetLayout);
	descriptorSet = vulkan->device.allocateDescriptorSets(allocateInfo)[0];
	auto writeSet = vk::WriteDescriptorSet()
		.setDstSet(descriptorSet)								// DescriptorSet dstSet;
		.setDstBinding(0)										// uint32_t dstBinding;
		.setDstArrayElement(0)									// uint32_t dstArrayElement;
		.setDescriptorCount(1)									// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)	// DescriptorType descriptorType;
		.setPBufferInfo(&vk::DescriptorBufferInfo()				// const DescriptorBufferInfo* pBufferInfo;
			.setBuffer(uniform.buffer)							// Buffer buffer;
			.setOffset(0)											// DeviceSize offset;
			.setRange(uniform.size));							// DeviceSize range;
	vulkan->device.updateDescriptorSets(writeSet, nullptr);
}

void LightUniforms::destroy()
{
	uniform.destroy();
	if (descriptorSetLayout) {
		vulkan->device.destroyDescriptorSetLayout(descriptorSetLayout);
		descriptorSetLayout = nullptr;
	}
}
