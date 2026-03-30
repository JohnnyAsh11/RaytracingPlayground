#ifndef __RANDOM_HLSLI_
#define __RANDOM_HLSLI_

#define PI 3.14159f

float rand(float2 uv)
{
    // Using random numerical constants from example code in lecture.
    return frac(sin(dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
}


float rand2(float2 uv)
{
    return float2(
        rand(uv),
        rand(uv.yx)
    );
}

float3 RandomCosineWeightedHemisphere(float u0, float u1, float3 unitNormal)
{
    float a = u0 * 2 - 1;
    float b = sqrt(1 - a * a);
    float phi = 2.0f * PI * u1;

    float x = unitNormal.x + b * cos(phi);
    float y = unitNormal.y + b * sin(phi);
    float z = unitNormal.z + a;

	// float pdf = a / PI;
    return float3(x, y, z);
}

#endif //__RANDOM_HLSLI_