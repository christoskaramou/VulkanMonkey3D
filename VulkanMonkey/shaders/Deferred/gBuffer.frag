#version 450
#extension GL_GOOGLE_include_directive : require

#include "../Common/common.glsl"

layout (set = 1, binding = 0) uniform sampler2D bcSampler; // BaseColor
layout (set = 1, binding = 1) uniform sampler2D mrSampler; // MetallicRoughness
layout (set = 1, binding = 2) uniform sampler2D nSampler;  // Normal
layout (set = 1, binding = 3) uniform sampler2D oSampler;  // Occlusion
layout (set = 1, binding = 4) uniform sampler2D eSampler;  // Emissive

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec4 baseColorFactor;
layout (location = 4) in vec3 emissiveFactor;
layout (location = 5) in vec4 metRoughAlphacutOcl;
layout (location = 6) in vec4 velocity;
layout (location = 7) in vec4 inWorldPos;

layout (location = 0) out float outDepth;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec4 outAlbedo;
layout (location = 3) out vec3 outMetRough;
layout (location = 4) out vec4 outVelocity;

void main() {
	float alpha = texture(bcSampler, inUV).a;
	//if(alpha < metRoughAlphacutOcl.z)
	if(alpha < 0.9)
		discard;

	float ao = texture(oSampler, inUV).r;

	outDepth = gl_FragCoord.z;
	outNormal = getNormal(inWorldPos.xyz, nSampler, inNormal, inUV);
	vec3 color = texture(bcSampler, inUV).xyz + inColor;
	outAlbedo = vec4(color * ao, alpha) * baseColorFactor
				+ vec4(texture(eSampler, inUV).xyz * emissiveFactor, 0.0);
	outMetRough = vec3(0.0, texture(mrSampler, inUV).y, texture(mrSampler, inUV).z);
	outVelocity = velocity;
}
