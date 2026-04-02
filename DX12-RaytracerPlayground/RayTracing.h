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
	// Stores window width/height data for ease of use.
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

	/// <summary>
	/// Initializes the RayTracing pipeline for the application.
	/// </summary>
	HRESULT Initialize(
		unsigned int outputWidth,
		unsigned int outputHeight,
		std::wstring raytracingShaderLibraryFile);
	
	/// <summary>
	/// Resizes the raytracing UAVs on window resizing.
	/// </summary>
	void ResizeOutputUAV(
		unsigned int outputWidth,
		unsigned int outputHeight);

	/// <summary>
	/// Performs the actual work for raytracing.  Results stored in RaytracingOutput resource.
	/// </summary>
	void Raytrace(
		std::shared_ptr<Camera> camera, 
		Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer,
		unsigned int a_uCubemapIndex = -1);

	/// <summary>
	/// Readys the raytraced frame for presentation.  Performs all filtering post processes if enabled.
	/// </summary>
	void FramePostProcess(
		Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer,
		D3D12_RESOURCE_BARRIER outputBarriers[2]);

	/// <summary>
	/// Generates a buffer containing cbuffer style data for entities in the scene.
	/// </summary>
	void CreateEntityDataBuffer(std::vector<std::shared_ptr<Entity>> scene);

	/// <summary>
	/// Creates the BLAS for a passed in Mesh.
	/// </summary>
	MeshRayTracingData CreateBottomLevelAccelerationStructureForMesh(Mesh* mesh);

	/// <summary>
	/// Creates a TLAS for all entities in a collection/scene.
	/// </summary>
	void CreateTopLevelAccelerationStructureForScene(std::vector<std::shared_ptr<Entity>> scene);

	// - - INITIALIZATION HELPER FUNCTIONS - -
	/// <summary>
	/// Initializes the Bilateral Filter compute shader pipeline for raytraced frames.
	/// </summary>
	void CreateBilateralFilterPipeline();
	/// <summary>
	/// Creates the root signature used for raytracing.
	/// </summary>
	void CreateRaytracingRootSignatures();
	/// <summary>
	/// Creates the (raytracing) pipeline state object used for the raytracing pipeline.
	/// </summary>
	void CreateRaytracingPipelineState(std::wstring raytracingShaderLibraryFile);
	/// <summary>
	/// Generates the shader tables containing the RayGen/Miss/ClosestHit shaders.
	/// </summary>
	void CreateShaderTables();
	/// <summary>
	/// Creates all necessary UAV resources for Raytracing pipeline.
	/// </summary>
	void CreateRaytracingOutputUAV(unsigned int width, unsigned int height);
}

#endif //__RAYTRACING_H_