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

#version 450
#extension GL_GOOGLE_include_directive : require

#include "../Common/tonemapping.glsl"

layout (set = 0, binding = 0) uniform sampler2D frameSampler;
layout (set = 0, binding = 1) uniform sampler2D gaussianVerticalSampler;
layout(push_constant) uniform Constants { float brightness; float intensity; float range; float exposure; float useTonemap; } values;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main() 
{
	vec4 sceneColor = texture(frameSampler, inUV);
	vec4 bloom = vec4(texture(gaussianVerticalSampler, inUV).xyz, 0.0);

	outColor = sceneColor + bloom * values.intensity;
	if (values.useTonemap > 0.5)
		outColor.xyz = SRGBtoLINEAR(TonemapFilmic(outColor.xyz, values.exposure));
}