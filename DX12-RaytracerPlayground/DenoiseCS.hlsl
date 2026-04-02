#include "random.hlsli"

SamplerState Sampler : register(s0);

#define MAX_FRAME_HISTORY 4

cbuffer TextureIndices : register(b0)
{
    unsigned int RaytracedFrameIndex;
    unsigned int FilterOutputIndex;
    
    unsigned int PastFrame1;
    unsigned int PastFrame2;
    unsigned int PastFrame3;
    unsigned int PastFrame4;
}

float4 LoadPixelFromHistory(unsigned int index, uint3 threadId)
{
    RWTexture2D<unorm float4> pastFrame = ResourceDescriptorHeap[PastFrame1];
    return pastFrame.Load(threadId);
}

[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    RWTexture2D<unorm float4> raytraceOutput = ResourceDescriptorHeap[RaytracedFrameIndex];
    RWTexture2D<unorm float4> filterOutput = ResourceDescriptorHeap[FilterOutputIndex];
    float4 currentPixel = raytraceOutput.Load(threadId);
        
    float4 totalPreviousFrames = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    totalPreviousFrames += LoadPixelFromHistory(PastFrame1, threadId);
    totalPreviousFrames += LoadPixelFromHistory(PastFrame2, threadId);
    totalPreviousFrames += LoadPixelFromHistory(PastFrame3, threadId);
    totalPreviousFrames += LoadPixelFromHistory(PastFrame4, threadId);
    
    filterOutput[threadId.xy] = (currentPixel + totalPreviousFrames) / (MAX_FRAME_HISTORY + 1);
    //filterOutput[threadId.xy] = currentPixel;
}