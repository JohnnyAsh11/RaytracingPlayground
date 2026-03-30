#include "Graphics.h"
#include "WICTextureLoader.h"
#include "ResourceUploadBatch.h"

#include <vector>
#include <dxgi1_6.h>

// Telling the drivers to use high-performance GPU in multi-GPU systems:
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; // NVIDIA
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1; // AMD
}

namespace Graphics
{
	// Anonymous namespace to hold internal values.
	namespace
	{
		unsigned int currentBackBufferIndex = 0;

		bool apiInitialized = false;
		bool supportsTearing = false;
		bool vsyncDesired = false;
		BOOL isFullscreen = false;

		D3D_FEATURE_LEVEL featureLevel{};

		// Descriptor heap management
		SIZE_T cbvSrvDescriptorHeapIncrementSize = 0;
		unsigned int cbvDescriptorOffset = 0;

		// CB upload heap management
		UINT64 cbUploadHeapSizeInBytes = 0;
		UINT64 cbUploadHeapOffsetInBytes = 0;
		void* cbUploadHeapStartAddress = 0;

		unsigned int srvDescriptorOffset = maxConstantBuffers; // Assume SRVs start after CBVs
		// Texture resources we need to keep alive
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textures;
	}
}

HRESULT Graphics::Initialize(unsigned int windowWidth, unsigned int windowHeight, HWND windowHandle, bool vsyncIfPossible)
{
	if (apiInitialized)
	{
		return E_FAIL;
	}
	vsyncDesired = vsyncIfPossible;

	// Ensuring Vsync is supported.
	Microsoft::WRL::ComPtr<IDXGIFactory5> factory;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
	{
		BOOL tearingSupported = false;
		HRESULT featureCheck = factory->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&tearingSupported,
			sizeof(tearingSupported));
		supportsTearing = SUCCEEDED(featureCheck) && tearingSupported;
	}

	// Enabling the debug layer if in debug mode.
#if defined (DEBUG) || defined ( _DEBUG)
	ID3D12Debug* debugController;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
	debugController->EnableDebugLayer();
#endif

	// Creating the Graphics Device object.
	// Also creating an HResult for error checking throughout this function.
	HRESULT hr = D3D12CreateDevice(
		0,									  
		D3D_FEATURE_LEVEL_11_0,				  
		IID_PPV_ARGS(Device.GetAddressOf())); 
	if (FAILED(hr))
	{
		return hr;
	}

	// Determining the max feature level of the hardware.
	D3D_FEATURE_LEVEL levelsToCheck[] = 
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_2
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS levels = {};
	levels.pFeatureLevelsRequested = levelsToCheck;
	levels.NumFeatureLevels = ARRAYSIZE(levelsToCheck);
	hr = Device->CheckFeatureSupport(
		D3D12_FEATURE_FEATURE_LEVELS,
		&levels,
		sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS));
	featureLevel = levels.MaxSupportedFeatureLevel; 
	if (FAILED(hr))
	{
		return hr;
	}

#if defined (DEBUG) || defined ( _DEBUG)
	// Set up callback for debug messages:
	Device->QueryInterface(IID_PPV_ARGS(&InfoQueue));
#endif

	// Setting up D3D12 command allocator/queue/list
	for (int i = 0; i < NumBackBuffers; i++)
	{
		// Set up allocator
		hr = Device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(CommandAllocator[i].GetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}
	}

	// Command queue
	D3D12_COMMAND_QUEUE_DESC qDesc = {};
	qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	hr = Device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(CommandQueue.GetAddressOf()));
	if (FAILED(hr))
	{
		return hr;
	}

	// Command list
	hr = Device->CreateCommandList(
		0,							
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		CommandAllocator[0].Get(),	
		0,							
		IID_PPV_ARGS(CommandList.GetAddressOf()));
	if (FAILED(hr))
	{
		return hr;
	}

	// Creating the swap chain:
	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	swapDesc.BufferCount = NumBackBuffers;
	swapDesc.BufferDesc.Width = windowWidth;
	swapDesc.BufferDesc.Height = windowHeight;
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.Flags = supportsTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	swapDesc.OutputWindow = windowHandle;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapDesc.Windowed = true;

	// Create the factory obj used to create the swap chain.
	Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;
	CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
	hr = dxgiFactory->CreateSwapChain(
		CommandQueue.Get(), &swapDesc, SwapChain.GetAddressOf());
	if (FAILED(hr))
	{
		return hr;
	}

	// Create a fence for basic synchronization:
	hr = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(WaitFence.GetAddressOf()));
	WaitFenceEvent = CreateEventEx(0, 0, 0, EVENT_ALL_ACCESS);
	if (FAILED(hr))
	{
		return hr;
	}

	// At this point the API has been initialized!
	apiInitialized = true;

	// Create descriptor heaps for back buffers and the depth buffer:
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = NumBackBuffers;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(RTVHeap.GetAddressOf()));
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(DSVHeap.GetAddressOf()));

	// Initializing the CBV/SRV descriptor heap and the CB upload heap for future use.
	// 
	// CBV/SRV heap description.
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
	cbvSrvHeapDesc.NumDescriptors = maxConstantBuffers + MaxTextureDescriptors;
	cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(CBVSRVDescriptorHeap.GetAddressOf()));

	cbvSrvDescriptorHeapIncrementSize = (SIZE_T)Device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// Ensure following of the 256 byte alignment requirement.
	cbUploadHeapSizeInBytes = (UINT64)(256 * maxConstantBuffers);

	// Creating the upload heap and buffer.
	// Enclosed in scope for cleanliness
	D3D12_HEAP_PROPERTIES cbUploadHeapProps = {};
	D3D12_RESOURCE_DESC cbUploadBufferDesc = {};
	{
		// Upload heap properties.
		cbUploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		cbUploadHeapProps.CreationNodeMask = 1;
		cbUploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		cbUploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		cbUploadHeapProps.VisibleNodeMask = 1;

		// Upload buffer description.
		cbUploadBufferDesc.Alignment = 0;
		cbUploadBufferDesc.DepthOrArraySize = 1;
		cbUploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		cbUploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		cbUploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		cbUploadBufferDesc.Height = 1;
		cbUploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		cbUploadBufferDesc.MipLevels = 1;
		cbUploadBufferDesc.SampleDesc.Count = 1;
		cbUploadBufferDesc.SampleDesc.Quality = 0;
		cbUploadBufferDesc.Width = cbUploadHeapSizeInBytes;
	}

	// Creating the ring buffer.
	Device->CreateCommittedResource(
		&cbUploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&cbUploadBufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
		0,
		IID_PPV_ARGS(CBUploadHeap.GetAddressOf()));

	// Calling resize buffers to create back buffers and depth buffer.
	ResizeBuffers(windowWidth, windowHeight);
	WaitForGPU();	// Ensuring all GPU work is completed before continuing.
	return S_OK;
}

void Graphics::ShutDown() {}

void Graphics::ResizeBuffers(unsigned int width, unsigned int height)
{
	// Ensure graphics API is initialized:
	if (!apiInitialized)
	{
		return;
	}

	// Ensure that there is nothing being done on the GPU.
	WaitForGPU();

	// Release the current back buffers:
	for (unsigned int i = 0; i < NumBackBuffers; i++)
	{
		BackBuffers[i].Reset();
	}

	// Resize the swap chain:
	SwapChain->ResizeBuffers(
		NumBackBuffers,
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		supportsTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
	SIZE_T RTVDescriptorSize = (SIZE_T)Device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Setting up the new back buffers and their RTVs.
	for (unsigned int i = 0; i < NumBackBuffers; i++)
	{
		// Grab this buffer from the swap chain:
		SwapChain->GetBuffer(i, IID_PPV_ARGS(BackBuffers[i].GetAddressOf()));

		// Create a handle for it:
		RTVHandles[i] = RTVHeap->GetCPUDescriptorHandleForHeapStart();
		RTVHandles[i].ptr += RTVDescriptorSize * (size_t)i;

		// Finally create the render target view:
		Device->CreateRenderTargetView(BackBuffers[i].Get(), 0, RTVHandles[i]);
	}

	// Clear the depth buffer and recreate it to match the new size.
	// Also enclosed in scope for cleanliness.
	{
		DepthBuffer.Reset();

		// Describe the depth stencil buffer resource:
		D3D12_RESOURCE_DESC depthBufferDesc = {};
		depthBufferDesc.Alignment = 0;
		depthBufferDesc.DepthOrArraySize = 1;
		depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthBufferDesc.Height = height;
		depthBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthBufferDesc.MipLevels = 1;
		depthBufferDesc.SampleDesc.Count = 1;
		depthBufferDesc.SampleDesc.Quality = 0;
		depthBufferDesc.Width = width;

		// Describe the clear value that will most often be used.
		D3D12_CLEAR_VALUE clear = {};
		clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		clear.DepthStencil.Depth = 1.0f;
		clear.DepthStencil.Stencil = 0;

		// Describe the memory heap it will use.
		D3D12_HEAP_PROPERTIES props = {};
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.CreationNodeMask = 1;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		props.Type = D3D12_HEAP_TYPE_DEFAULT;
		props.VisibleNodeMask = 1;

		// Create its resource and housing heap.
		Device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&depthBufferDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clear,
			IID_PPV_ARGS(DepthBuffer.GetAddressOf()));

		// Finally, recreate the depth stencil view:
		DSVHandle = DSVHeap->GetCPUDescriptorHandleForHeapStart();
		Device->CreateDepthStencilView(
			DepthBuffer.Get(),
			0,
			DSVHandle);
	}

	// Reset back to the first buffer
	currentBackBufferIndex = 0;

	// Reseting the command list and allocators.
	CommandList->Close();
	for (unsigned int i = 0; i < NumBackBuffers; i++)
	{
		CommandAllocator[i]->Reset();
	}
	CommandList->Reset(CommandAllocator[0].Get(), 0);

	// Checking for fullscreen state.
	SwapChain->GetFullscreenState(&isFullscreen, 0);
	WaitForGPU();
}

void Graphics::PrintDebugMessages()
{
	// -------------------------------------------------------------
	// Implementation taken from Professor Chris Cascioli from 
	// Rochester Institute of Technology's Computer Graphics 
	// course, with some modifications.
	// -------------------------------------------------------------
	
	// Do we actually have an info queue (usually in debug mode)
	if (!InfoQueue)
		return;

	// Any messages?
	UINT64 messageCount = InfoQueue->GetNumStoredMessages();
	if (messageCount == 0)
		return;

	// Loop and print messages
	for (UINT64 i = 0; i < messageCount; i++)
	{
		// Get the size so we can reserve space
		size_t messageSize = 0;
		InfoQueue->GetMessage(i, 0, &messageSize);

		// Reserve space for this message
		D3D12_MESSAGE* message = (D3D12_MESSAGE*)malloc(messageSize);
		InfoQueue->GetMessage(i, message, &messageSize);

		// Print and clean up memory
		if (message)
		{
			// Color code based on severity
			switch (message->Severity)
			{
			case D3D12_MESSAGE_SEVERITY_CORRUPTION:
			case D3D12_MESSAGE_SEVERITY_ERROR:
				printf("\x1B[91m"); break; // RED

			case D3D12_MESSAGE_SEVERITY_WARNING:
				printf("\x1B[93m"); break; // YELLOW

			case D3D12_MESSAGE_SEVERITY_INFO:
			case D3D12_MESSAGE_SEVERITY_MESSAGE:
				printf("\x1B[96m"); break; // CYAN
			}

			printf("%s\n\n", message->pDescription);
			free(message);

			// Reset color
			printf("\x1B[0m");
		}
	}

	// Clear any messages we've printed
	InfoQueue->ClearStoredMessages();
}

void Graphics::WaitForGPU()
{
	CpuCounter++;
	CommandQueue->Signal(WaitFence.Get(), CpuCounter);

	// Essentially ensuring that the GPU and CPU are caught up with each other.
	if (WaitFence->GetCompletedValue() < CpuCounter)
	{
		// Otherwise, wait for catch up.
		WaitFence->SetEventOnCompletion(CpuCounter, WaitFenceEvent);
		WaitForSingleObject(WaitFenceEvent, INFINITE);
	}

	GpuCounter = CpuCounter;
}

void Graphics::ResetAllocatorAndCommandList()
{
	CommandAllocator[SwapChainIndex()]->Reset();
	CommandList->Reset(CommandAllocator[SwapChainIndex()].Get(), 0);
}

void Graphics::CloseAndExecuteCommandList()
{
	// Close the current list and execute it.
	CommandList->Close();
	ID3D12CommandList* lists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(1, lists);
}

unsigned int Graphics::SwapChainIndex() { return currentBackBufferIndex; }
void Graphics::AdvanceSwapChainIndex()
{
	CpuCounter++;
	CommandQueue->Signal(WaitFence.Get(), CpuCounter);

	UINT64 frames = CpuCounter - GpuCounter;
	if (frames >= NumBackBuffers)
	{
		// If CPU has gone too far, wait for a free frame.
		if (WaitFence->GetCompletedValue() < GpuCounter + 1)
		{
			WaitFence->SetEventOnCompletion(GpuCounter + 1, WaitFenceEvent);
			WaitForSingleObject(WaitFenceEvent, INFINITE);
		}
		GpuCounter++;
	}

	// Setting the new buffer indices.
	currentBackBufferIndex++;
	currentBackBufferIndex %= NumBackBuffers;
}

unsigned int Graphics::LoadTexture(const wchar_t* file, bool generateMips)
{
	DirectX::ResourceUploadBatch upload(Device.Get());
	upload.Begin();

	// Attempt to create the texture:
	Microsoft::WRL::ComPtr<ID3D12Resource> texture;
	DirectX::CreateWICTextureFromFile(Device.Get(), upload, file, texture.GetAddressOf(), generateMips);

	// Perform the upload and wait for it to finish before moving on.
	std::future<void> finish = upload.End(CommandQueue.Get());
	finish.wait();

	// Saving the ComPtr so that the texture resources stay alive for the duration of the program.
	textures.push_back(texture);

	// Saving the index of this descriptor and incrementing the overall offset:
	unsigned int srvIndex = srvDescriptorOffset;
	srvDescriptorOffset++;

	// Getting the location of this SRV by Offsetting 
	// the ptr by the product of the index and increment size.
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle(
		CBVSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	srvHandle.ptr = srvHandle.ptr + (srvIndex * cbvSrvDescriptorHeapIncrementSize);

	// Creating the SRV itself:
	Device->CreateShaderResourceView(
		texture.Get(),     // Resource ptr.
		nullptr,           // Base SRV description.
		srvHandle          // Previous calculated destination.
	);

	// Send back the index of the descriptor:
	return srvIndex;
}

D3D12_GPU_DESCRIPTOR_HANDLE Graphics::IncrementCBufferGetHandle(void* data, unsigned int dataSizeInBytes)
{
	// Determining how much space is needed since each chunk is 256 bytes.
	SIZE_T reservationSize = (SIZE_T)dataSizeInBytes;
	reservationSize = (reservationSize + 255) / 256 * 256; // Integer division trick

	// Ensure this upload will fit in the remaining space. If not, reset to beginning.
	if (cbUploadHeapOffsetInBytes + reservationSize >= cbUploadHeapSizeInBytes)
	{
		cbUploadHeapOffsetInBytes = 0;
	}

	// Where in the upload heap will this data go?
	D3D12_GPU_VIRTUAL_ADDRESS virtualGPUAddress = 
		CBUploadHeap->GetGPUVirtualAddress() + cbUploadHeapOffsetInBytes;
	// Copying data to the upload heap:
	{
		// Getting the mapped address and then adding the offset onto it.
		CBUploadHeap->Map(0, nullptr, &cbUploadHeapStartAddress);
		uint8_t* uploadAddress = static_cast<uint8_t*>(cbUploadHeapStartAddress) + cbUploadHeapOffsetInBytes;

		// Perform the mem copy to put new data into this part of the heap
		memcpy(uploadAddress, data, dataSizeInBytes);

		// Increment the offset and loop back to the beginning if necessary,
		// allowing us to treat the upload heap like a ring buffer
		cbUploadHeapOffsetInBytes += reservationSize;
		if (cbUploadHeapOffsetInBytes >= cbUploadHeapSizeInBytes)
		{
			cbUploadHeapOffsetInBytes = 0;
		}
	}

	// Create a CBV for this section of the heap:
	{
		// Calculate the CPU and GPU side handles for this descriptor
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = CBVSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = CBVSRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

		// Offset each by based on how many descriptors we've used 
		// already, which gives us the location of the next CBV descriptor in the heap.
		cpuHandle.ptr += cbvDescriptorOffset * cbvSrvDescriptorHeapIncrementSize;
		gpuHandle.ptr += cbvDescriptorOffset * cbvSrvDescriptorHeapIncrementSize;

		// Creating the description of the new CBV.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = virtualGPUAddress;
		cbvDesc.SizeInBytes = (UINT)reservationSize;

		// Creating the actual CBV.
		Device->CreateConstantBufferView(&cbvDesc, cpuHandle);

		// Increment the offset and loop back to the beginning if necessary.
		cbvDescriptorOffset++;
		if (cbvDescriptorOffset >= maxConstantBuffers)
		{
			cbvDescriptorOffset = 0;
		}

		return gpuHandle;
	}
}

Microsoft::WRL::ComPtr<ID3D12Resource> Graphics::CreateStaticBuffer(size_t dataStride, size_t dataCount, void* data)
{
	// -------------------------------------------------------------
	// Implementation taken from Professor Chris Cascioli from 
	// Rochester Institute of Technology's Computer Graphics 
	// course, with some modifications.
	// -------------------------------------------------------------

	// Creates a temporary command allocator and list so we don't
	// screw up any other ongoing work (since resetting a command allocator
	// cannot happen while its list is being executed). These ComPtrs will
	// be cleaned up automatically when they go out of scope.
	// Note: This certainly isn't efficient, but hopefully this only
	// happens during start - up. Otherwise, refactor this to use
	// the existing list and allocator(s).
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> localAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> localList;
	Device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(localAllocator.GetAddressOf()));
	Device->CreateCommandList(
		0,								// Which physical GPU will handle these tasks? 0 for single GPU setup
		D3D12_COMMAND_LIST_TYPE_DIRECT, // Type of command list
		localAllocator.Get(),			// The allocator for this list (to start)
		0,								// Initial pipeline state - none for now
		IID_PPV_ARGS(localList.GetAddressOf()));

	// The overall buffer we'll be creating
	Microsoft::WRL::ComPtr<ID3D12Resource> finalBuffer;

	// Describes the final heap
	D3D12_HEAP_PROPERTIES props = {};
	props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	props.CreationNodeMask = 1;
	props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	props.Type = D3D12_HEAP_TYPE_DEFAULT;
	props.VisibleNodeMask = 1;
	D3D12_RESOURCE_DESC desc = {};
	desc.Alignment = 0;
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Height = 1; // Assuming this is a regular buffer, not a texture
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Width = dataStride * dataCount; // Size of the buffer

	// Note that even though we're starting this buffer in the "common" resource
	// state, it will be implicitly transitioned to the "copy destination" state
	// when used for a copy operation below. For more info, see:
	// https://learn.microsoft.com/en - us/windows/win32/direct3d12/user - mode- heap - synchronization#multi - queue - resource - access
	Device->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COMMON, // Must start in "common" state to avoid warning
		0,
		IID_PPV_ARGS(finalBuffer.GetAddressOf()));

	// Now create an intermediate upload heap for CPU - >GPU copy of initial data
	D3D12_HEAP_PROPERTIES uploadProps = {};
	uploadProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadProps.CreationNodeMask = 1;
	uploadProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadProps.VisibleNodeMask = 1;

	Microsoft::WRL::ComPtr<ID3D12Resource>uploadBuffer;
	Device->CreateCommittedResource(&uploadProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		0,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

	// Do a straight map/memcpy/unmap
	void* gpuAddress = 0;
	uploadBuffer->Map(0, 0, &gpuAddress);
	memcpy(gpuAddress, data, dataStride * dataCount);
	uploadBuffer->Unmap(0, 0);

	// Copy the whole buffer from uploadheap to vert buffer
	localList->CopyResource(finalBuffer.Get(), uploadBuffer.Get());

	// Transition the buffer to generic read (was implicitly transitioned to copy_dest above)
	D3D12_RESOURCE_BARRIER rb = {};
	rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	rb.Transition.pResource = finalBuffer.Get();
	rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST; // Implicitly copy_dest now!
	rb.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	localList->ResourceBarrier(1, &rb);

	// Execute the local command list and wait for it to complete
	// before returning the final buffer
	localList->Close();
	ID3D12CommandList* list[] = { localList.Get() };
	CommandQueue->ExecuteCommandLists(1, list);

	WaitForGPU();
	return finalBuffer;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Graphics::CreateBuffer(
	UINT64 bufferSize, 
	D3D12_HEAP_TYPE heapType, 
	D3D12_RESOURCE_STATES state, 
	D3D12_RESOURCE_FLAGS flags, 
	UINT64 alignment, 
	void* data, 
	size_t dataSize)
{
	// -------------------------------------------------------------
	// Implementation taken from Professor Chris Cascioli from 
	// Rochester Institute of Technology's Computer Graphics 
	// course, with some modifications.
	// -------------------------------------------------------------
	
	// The final buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer;

	// Describe the heap
	D3D12_HEAP_PROPERTIES heapDesc = {};
	heapDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapDesc.CreationNodeMask = 1;
	heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapDesc.Type = heapType;
	heapDesc.VisibleNodeMask = 1;

	// Describe the resource
	D3D12_RESOURCE_DESC desc = {};
	desc.Alignment = 0;
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Flags = flags;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Height = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Width = bufferSize; // Size of the buffer

	// Create the buffer
	Device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &desc, state, 0, IID_PPV_ARGS(buffer.GetAddressOf()));

	// Do we need to copy data to the buffer?
	if (data && dataSize > 0 && dataSize <= bufferSize)
	{
		// We need to copy data into the final buffer, so create
		// an upload buffer to initially get the data
		D3D12_HEAP_PROPERTIES uploadProps = {};
		uploadProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		uploadProps.CreationNodeMask = 1;
		uploadProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		uploadProps.VisibleNodeMask = 1;

		// Remove any special flags on the initial description
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap;
		HRESULT hr = Device->CreateCommittedResource(
			&uploadProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COMMON,
			0,
			IID_PPV_ARGS(uploadHeap.GetAddressOf()));

		// Do a straight map/memcpy/unmap
		void* gpuAddress = 0;
		uploadHeap->Map(0, 0, &gpuAddress);
		memcpy(gpuAddress, data, dataSize);
		uploadHeap->Unmap(0, 0);

		// Create a local command list + allocator
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> localAllocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> localList;

		Device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(localAllocator.GetAddressOf()));

		Device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			localAllocator.Get(),
			0,
			IID_PPV_ARGS(localList.GetAddressOf()));

		// Copy the whole buffer from uploadheap to vert buffer
		localList->CopyResource(buffer.Get(), uploadHeap.Get());

		// Transition the buffer to generic read for the rest of the app lifetime (presumably)
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = buffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		localList->ResourceBarrier(1, &rb);

		// Execute the local command list and wait for it to complete
		// before returning the final buffer
		localList->Close();
		ID3D12CommandList* list[] = { localList.Get() };
		CommandQueue->ExecuteCommandLists(1, list);

		WaitForGPU();
	}

	// Return the final buffer
	return buffer;
}

void Graphics::ReserveDescriptorHeapSlot(D3D12_CPU_DESCRIPTOR_HANDLE* reservedCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE* reservedGPUHandle)
{
	// Get the actual heap start on both sides and offset to the next open SRV/UAV portion.
	// Similar to the handle calculation in IncrementCBufferGetHandle, 
	// but using srvDescriptorOffset instead of cbvDescriptorOffset.
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = CBVSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = CBVSRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	cpuHandle.ptr += (SIZE_T)srvDescriptorOffset * cbvSrvDescriptorHeapIncrementSize;
	gpuHandle.ptr += (SIZE_T)srvDescriptorOffset * cbvSrvDescriptorHeapIncrementSize;

	// Set the requested handles to the calculated values.
	if (reservedCPUHandle) { *reservedCPUHandle = cpuHandle; }
	if (reservedGPUHandle) { *reservedGPUHandle = gpuHandle; }

	// Update the overall offset if at least one handle was reserved by this function:
	if (reservedCPUHandle || reservedGPUHandle)
	{
		srvDescriptorOffset++;
	}
}

unsigned int Graphics::GetDescriptorIndex(D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
	// Getting the start of the heap.
	D3D12_GPU_DESCRIPTOR_HANDLE heapStart = CBVSRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// Getting the descriptor size from the device.
	UINT descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Calculating the index by seeing how many increments the handle is away from the start of the heap.
	return (unsigned int)((handle.ptr - heapStart.ptr) / descriptorSize);
}

bool Graphics::VsyncState() { return vsyncDesired || !supportsTearing || isFullscreen; }
std::wstring Graphics::APIName()
{
	switch (featureLevel)
	{
	case D3D_FEATURE_LEVEL_10_0: return L"D3D10";
	case D3D_FEATURE_LEVEL_10_1: return L"D3D10.1";
	case D3D_FEATURE_LEVEL_11_0: return L"D3D11";
	case D3D_FEATURE_LEVEL_11_1: return L"D3D11.1";
	case D3D_FEATURE_LEVEL_12_0: return L"D3D12";
	case D3D_FEATURE_LEVEL_12_1: return L"D3D12.1";
	case D3D_FEATURE_LEVEL_12_2: return L"D3D12.2";

	default: return L"Unknown";
	}
}