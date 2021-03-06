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

#ifndef PBR_H_
#define PBR_H_

#ifndef PI
#define PI 3.1415628
#endif

float D_GGX(float roughness, vec3 N, vec3 H)
{
#if 1
    float NoH = clamp(dot(N, H), 0.0001, 1.0);
    float m = roughness * roughness;
    float m2 = m * m;
    float d = (NoH * m2 - NoH) * NoH + 1.0;
    return m2 / (PI * d * d);
#else
	float NoH = clamp(dot(N, H), 0.001, 1.0);
	vec3 NxH = cross(N, H);
	float one_minus_NoH_squared = min(dot(NxH, NxH), 1.0);
	float linear_roughness = roughness * roughness;
	float a = NoH * linear_roughness;
	float k = linear_roughness / max(one_minus_NoH_squared + a * a, 0.0001);
	float d = k * k * (1.0 / PI);
	return d;
#endif
}

float G_schlick(float roughness, float NoV, float NoL)
{
    float r = roughness + 1.0;
    float k = r * r * (1.0 / 8.0);
    float V = NoV * (1.0 - k) + k;
    float L = NoL * (1.0 - k) + k;
    return 0.25 / max(V * L, 0.001); // 1 / (4 * NoV * NoL) is folded in here.
}

vec3 cook_torrance_specular(vec3 N, vec3 H, float NoL, float NoV, vec3 specular, float roughness)
{
    float D = D_GGX(roughness, N, H);
    float G = G_schlick(roughness, NoV, NoL);
    return specular * G * D;
}

vec3 fresnel(vec3 F0, float HoV)
{
	return mix(F0, vec3(1.0), pow((1.0 - HoV), 5.0));
}

vec3 fresnel_ibl(vec3 F0, float cos_theta, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
}

vec3 compute_F0(vec3 base_color, float metallic)
{
	return mix(vec3(0.04), base_color, metallic);
}

#endif