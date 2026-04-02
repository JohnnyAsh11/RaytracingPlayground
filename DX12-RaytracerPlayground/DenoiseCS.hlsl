#include "random.hlsli"

SamplerState Sampler : register(s0);

#define MAX_FRAME_HISTORY 3

cbuffer TextureIndices : register(b0)
{
    unsigned int RaytracedFrameIndex;
    unsigned int FilterOutputIndex;
    unsigned int PreviousFrameIndices[MAX_FRAME_HISTORY];
}

[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    RWTexture2D<unorm float4> raytraceOutput = ResourceDescriptorHeap[RaytracedFrameIndex];
    RWTexture2D<unorm float4> filterOutput = ResourceDescriptorHeap[FilterOutputIndex];
    float4 currentPixel = raytraceOutput.Load(threadId);
    
    RWTexture2D<unorm float4> prevFrame = ResourceDescriptorHeap[PreviousFrameIndices[2]];
    float4 prevFramePixel = prevFrame.Load(threadId);
    filterOutput[threadId.xy] = prevFramePixel;
        
    //float4 totalPreviousFrames = float4(0.0f, 0.0f, 0.0f, 1.0f);
    //for (int i = 0; i < MAX_FRAME_HISTORY; i++)
    //{
    //    RWTexture2D<unorm float4> prevFrame = ResourceDescriptorHeap[PreviousFrameIndices[i]];
    //    float4 prevFramePixel = prevFrame.Load(threadId);
        
    //    totalPreviousFrames += prevFramePixel;
    //}
    
    //filterOutput[threadId.xy] = (currentPixel + totalPreviousFrames) / (MAX_FRAME_HISTORY + 1);

}