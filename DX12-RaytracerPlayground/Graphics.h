#ifndef __GRAPHICS_H_
#define __GRAPHICS_H_

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl/client.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace Graphics
{
	// The number of back buffers in the swap chain.
	// Note that the more buffers, the more GPU memory is used.
	// Trade is that it can increase performance.
	const unsigned int NumBackBuffers = 3;

	// Core D3D12 API objects:
	inline Microsoft::WRL::ComPtr<ID3D12Device> Device;
	inline Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;

	// Command submission objects:
	inline Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator[NumBackBuffers];
	inline Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue;
	inline Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;

	// Rendering buffers & descriptors:
	inline Microsoft::WRL::ComPtr<ID3D12Resource> BackBuffers[NumBackBuffers];
	inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RTVHeap;
	inline D3D12_CPU_DESCRIPTOR_HANDLE RTVHandles[NumBackBuffers]{};

	inline Microsoft::WRL::ComPtr<ID3D12Resource> DepthBuffer;
	inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DSVHeap;
	inline D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle{};

	// Basic CPU/GPU sync:
	inline Microsoft::WRL::ComPtr<ID3D12Fence> WaitFence;
	inline HANDLE WaitFenceEvent = 0;
	inline UINT64 CpuCounter = 0;
	inline UINT64 GpuCounter = 0;

	// Debug Info Queue:
	inline Microsoft::WRL::ComPtr<ID3D12InfoQueue> InfoQueue;

	// Descriptor/CBuffer maximums:
	const unsigned int MaxTextureDescriptors = 100;
	const unsigned int maxConstantBuffers = 1000;
	inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CBVSRVDescriptorHeap;
	inline Microsoft::WRL::ComPtr<ID3D12Resource> CBUploadHeap;

	// General functions
	/// <summary>
	/// Initializes the graphics system for the program.
	/// </summary>
	HRESULT Initialize(unsigned int windowWidth, unsigned int windowHeight, HWND windowHandle, bool vsyncIfPossible);

	/// <summary>
	/// Shuts down and cleans up the graphics system and its associated resources.
	/// </summary>
	void ShutDown();

	/// <summary>
	/// Resizes the swap chain and its associated buffers to match a new width and height.
	/// </summary>
	void ResizeBuffers(unsigned int width, unsigned int height);

	/// <summary>
	/// Prints out any messages that have been stored in the D3D12 debug info queue to the debug output.
	/// </summary>
	void PrintDebugMessages();

	/// <summary>
	/// Pauses C++ code until GPU work is complete.
	/// </summary>
	void WaitForGPU();

	/// <summary>
	/// Resets allocator/command list for next frame.
	/// </summary>
	void ResetAllocatorAndCommandList();

	/// <summary>
	/// Executes the current contents of the command list on the GPU.
	/// </summary>
	void CloseAndExecuteCommandList();

	/// <summary>
	/// Gets the current swap chain index.
	/// </summary>
	unsigned int SwapChainIndex();

	/// <summary>
	/// Advances the swap chain index to the next index.
	/// </summary>
	void AdvanceSwapChainIndex();

	/// <summary>
	/// Loads in a texture resource from a file into the program.
	/// </summary>
	unsigned int LoadTexture(const wchar_t* file, bool generateMips = true);

	/// <summary>
	/// Loads in 6 textures to put together as a cubemap.  Returns the index of its descriptor.
	/// </summary>
	unsigned int LoadCubeMap(
		const wchar_t* a_pRight, 
		const wchar_t* a_pLeft, 
		const wchar_t* a_pUp, 
		const wchar_t* a_pDown, 
		const wchar_t* a_pFront, 
		const wchar_t* a_pBack);

	/// <summary>
	/// Fills the next available constant buffer with the passed in data, and returns a GPU descriptor handle to that buffer.
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE IncrementCBufferGetHandle(
		void* data,
		unsigned int dataSizeInBytes);

	/// <summary>
	/// Creates a static buffer with the passed in data and sizes.
	/// </summary>
	/// <param name="dataStride">Size of an index.</param>
	/// <param name="dataCount">Amount of data in buffer.</param>
	/// <param name="data">The data itself.</param>
	/// <returns>A ComPtr to the new static buffer resource.</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateStaticBuffer(
		size_t dataStride, size_t dataCount, void* data);

	/// <summary>
	/// Generic buffer creation function.
	/// More flexible than CreateStaticBuffer, 
	/// but requires more parameters to be passed in.
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(
		UINT64 bufferSize,
		D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		UINT64 alignment = 0,
		void* data = 0,
		size_t dataSize = 0);

	void ReserveDescriptorHeapSlot(
		D3D12_CPU_DESCRIPTOR_HANDLE* reservedCPUHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE* reservedGPUHandle);

	// Accessors:
	/// <summary>
	/// Gets the index of a descriptor in the CBV/SRV descriptor heap based on its GPU handle.
	/// </summary>
	unsigned int GetDescriptorIndex(D3D12_GPU_DESCRIPTOR_HANDLE handle);
	/// <summary>
	/// Gets whether or not Vsync is enabled.
	/// </summary>
	bool VsyncState();
	/// <summary>
	/// Gets the string name of the graphics API in use.
	/// </summary>
	std::wstring APIName();
}

#endif //__GRAPHICS_H_

