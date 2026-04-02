#ifndef __RAYTRACING_H_
#define __RAYTRACING_H_

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <string>
#include <vector>

#include "Mesh.h"
#include "Camera.h"
#include "Entity.h"
#include "Graphics.h"
#include "BufferStructs.h"

namespace RayTracing
{
	inline unsigned int Width;
	inline unsigned int Height;

	// Raytracing-specific versions of base DX12 objects
	inline Microsoft::WRL::ComPtr<ID3D12Device5> DXRDevice;
	inline Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> DXRCommandList;

	// Root signatures for basic raytracing
	inline Microsoft::WRL::ComPtr<ID3D12RootSignature> GlobalRaytracingRootSig;

	// Overall raytracing pipeline state object
	// This is similar to a regular PSO, but without the standard
	// rasterization pipeline stuff.  Also grabbing the properties
	// so we can get shader IDs out of it later.
	inline Microsoft::WRL::ComPtr<ID3D12StateObject> RaytracingPipelineStateObject;
	inline Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> RaytracingPipelineProperties;

	// Shader tables holding shaders for use during raytracing
	inline Microsoft::WRL::ComPtr<ID3D12Resource> RayGenTable;
	inline Microsoft::WRL::ComPtr<ID3D12Resource> MissTable;
	inline Microsoft::WRL::ComPtr<ID3D12Resource> HitGroupTable;

	// Accel structure requirements
	inline Microsoft::WRL::ComPtr<ID3D12Resource> TLASScratchBuffer;
	inline Microsoft::WRL::ComPtr<ID3D12Resource> TLASInstanceDescBuffer[Graphics::NumBackBuffers];
	inline Microsoft::WRL::ComPtr<ID3D12Resource> TLAS;
	inline D3D12_CPU_DESCRIPTOR_HANDLE TLASDescriptor_CPU{};
	inline D3D12_GPU_DESCRIPTOR_HANDLE TLASDescriptor_GPU{};

	// Actual raytracing output resource:
	inline Microsoft::WRL::ComPtr<ID3D12Resource> RaytracingOutput;
	inline D3D12_CPU_DESCRIPTOR_HANDLE RaytracingOutputUAV_CPU;
	inline D3D12_GPU_DESCRIPTOR_HANDLE RaytracingOutputUAV_GPU;

	// Compute shader filteration output:
	inline Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pBilateralFilterRootSig;
	inline Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pBilateralFilterPso;
	inline Microsoft::WRL::ComPtr<ID3D12Resource> FilteringOutput;
	inline D3D12_CPU_DESCRIPTOR_HANDLE FilteringOutputUAV_CPU;
	inline D3D12_GPU_DESCRIPTOR_HANDLE FilteringOutputUAV_GPU;
	inline bool m_bFilterOn = true;

	// Starter values for Bilateral Filter.
	inline float SigmaSpatial = 2.0f;
	inline float SigmaColor = 0.1f;
	inline int KernelRadius = 2.0f;

	// Starter values for path tracing.
	inline int RaysPerPixel = 15;
	inline int RecursionDepth = 10;

	// Buffer for bindless per-entity data
	inline Microsoft::WRL::ComPtr<ID3D12Resource> EntityDataStructuredBuffer;
	inline D3D12_CPU_DESCRIPTOR_HANDLE EntityDataUAV_CPU{};
	inline D3D12_GPU_DESCRIPTOR_HANDLE EntityDataUAV_GPU{};

	HRESULT Initialize(
		unsigned int outputWidth,
		unsigned int outputHeight,
		std::wstring raytracingShaderLibraryFile);
	
	void ResizeOutputUAV(
		unsigned int outputWidth,
		unsigned int outputHeight);

	void Raytrace(
		std::shared_ptr<Camera> camera, 
		Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer,
		unsigned int a_uCubemapIndex = -1);

	void CreateEntityDataBuffer(std::vector<std::shared_ptr<Entity>> scene);

	// Helpers for creating acceleration structures
	MeshRayTracingData CreateBottomLevelAccelerationStructureForMesh(Mesh* mesh);

	void CreateTopLevelAccelerationStructureForScene(std::vector<std::shared_ptr<Entity>> scene);

	// Helper functions for each initalization step
	void CreateBilateralFilterPipeline();
	void CreateRaytracingRootSignatures();
	void CreateRaytracingPipelineState(std::wstring raytracingShaderLibraryFile);
	void CreateShaderTables();
	void CreateRaytracingOutputUAV(unsigned int width, unsigned int height);
}

#endif //__RAYTRACING_H_