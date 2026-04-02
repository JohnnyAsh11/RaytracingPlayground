#include "random.hlsli"

cbuffer TextureIndices : register(b0)
{
    unsigned int RaytracedFrameIndex;
    unsigned int FilterOutputIndex;
    float SigmaSpatial;
    float SigmaColor;
    int KernelRadius;
}

[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{    
    RWTexture2D<unorm float4> raytraceOutput = ResourceDescriptorHeap[RaytracedFrameIndex];
    RWTexture2D<unorm float4> filterOutput = ResourceDescriptorHeap[FilterOutputIndex];
    float4 currentPixel = raytraceOutput.Load(threadId);
    
    int2 pixelIndex = threadId.xy;
    float3 center = currentPixel.rgb;

    float3 totalColor = 0.0;
    float totalWeight = 0.0;

    for (int y = -KernelRadius; y <= KernelRadius; y++)
    {
        for (int x = -KernelRadius; x <= KernelRadius; x++)
        {
            // Getting the current iteration's pixel color.
            int2 offset = int2(x, y);
            float3 sample = raytraceOutput[pixelIndex + offset].rgb;

            // Calculating the spatial weight.
            float dist2 = dot(float2(x, y), float2(x, y));
            float wSpatial = exp(-dist2 / (2.0 * SigmaSpatial * SigmaSpatial));

            // Calculating the color weight.
            float3 diff = sample - center;
            float colorDist2 = dot(diff, diff);
            float wColor = exp(-colorDist2 / (2.0 * SigmaColor * SigmaColor));

            // Calculating the final weight.
            float weight = wSpatial * wColor;

            // Adding everything together.
            totalColor += sample * weight;
            totalWeight += weight;
        }
    }

    filterOutput[pixelIndex] = float4(totalColor / totalWeight, 1.0);
}