#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 projection;
	mat4 view;
	mat4 model;
	vec4 baseColorFactor;
	vec4 emissiveFactor;
	float metallicFactor;
	float roughnessFactor;
	float alphaCutoff;
	float hasAlphaMap;
	vec4 dummy;
	mat4 previousModel;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;
layout(location = 5) in vec4 inColor;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outTangent;
layout (location = 3) out vec3 outBitangent;
layout (location = 4) out vec3 outColor;
layout (location = 5) out vec4 baseColorFactor;
layout (location = 6) out vec3 emissiveFactor;
layout (location = 7) out vec4 metRoughAlphacut;
layout (location = 8) out vec4 velocity;

void main() 
{
	vec4 inPos = vec4(inPosition, 1.0f);
	vec4 viewPos = ubo.view * ubo.model * inPos;
	gl_Position = ubo.projection * viewPos;
	
	// UV
	outUV = inTexCoords;

	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(ubo.model)));
	outNormal = mNormal * inNormal;

	// Tangent in world space
	outTangent = mNormal * inTangent;

	// Bitangent in world space
	outBitangent = mNormal * inBitangent;

	// Color (not in use)	
	outColor = inColor.rgb;

	// Factors
	baseColorFactor = ubo.baseColorFactor;
	emissiveFactor = ubo.emissiveFactor.xyz;
	metRoughAlphacut = vec4(ubo.metallicFactor, ubo.roughnessFactor, ubo.alphaCutoff, ubo.hasAlphaMap);

	// Velocity
	velocity = (ubo.model * inPos) - (ubo.previousModel * inPos);
	velocity.w = 1.0;
}
