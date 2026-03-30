#ifndef __BUFFERSTRUCTS_H_
#define __BUFFERSTRUCTS_H_

#include <DirectXMath.h>

// Root constants for bindless resources
struct RayTracingDrawData
{
	unsigned int SceneDataConstantBufferIndex;
	unsigned int EntityDataDescriptorIndex;
	unsigned int SceneTLASDescriptorIndex;
	unsigned int OutputUAVDescriptorIndex;
	unsigned int SkyboxDescriptorIndex;
};

// Overall scene data for ray tracing (constant buffer)
struct RayTracingSceneData
{
	DirectX::XMFLOAT4X4 InverseViewProjection;
	// - -
	DirectX::XMFLOAT3 CameraPosition;
	unsigned int RaysPerPixel;
	// - -
	unsigned int RecursionDepth;
	DirectX::XMFLOAT3 padding;
};

// Per-entity information (geom, material, etc.)
// - Multiple sets of these will be stored in a structured buffer
// - Materials *could* be separated out into their own buffer
// to cut down on data repetition
struct RayTracingEntityData
{
	DirectX::XMFLOAT4 Color;
	// - -
	unsigned int VertexBufferDescriptorIndex;
	unsigned int IndexBufferDescriptorIndex;
	unsigned int AlbedoIndex = (unsigned int)-1;
	unsigned int NormalIndex = (unsigned int)-1;
	// - -
	unsigned int RoughnessIndex = (unsigned int)-1;
	unsigned int MetallicIndex = (unsigned int)-1;
	float Roughness = 1.0f;
	float Metalness = 0.0f;
};

#endif //__BUFFERSTRUCTS_H_
