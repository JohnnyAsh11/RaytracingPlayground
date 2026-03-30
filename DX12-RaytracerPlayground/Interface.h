#ifndef __INTERFACE_H_
#define __INTERFACE_H_

#include "Window.h"
#include "Graphics.h"
#include "Input.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx12.h"
#include "ImGui/imgui_impl_win32.h"

namespace Interface
{
	/// <summary>
	/// Sets the UI frame for the next render.
	/// Function Pointer should be building the UI frame!
	/// </summary>
	inline void SetFrame(void(*a_pUiBuild)(), float a_fDeltaTime)
	{
		// Updating all of the frame to frame logic for ImGui:
		// - Input
		// - Window Size
		// - Delta Time
		ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = a_fDeltaTime;
		io.DisplaySize.x = (float)Window::GetWidth();	
		io.DisplaySize.y = (float)Window::GetHeight();

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		Input::SetKeyboardCapture(io.WantCaptureKeyboard);
		Input::SetMouseCapture(io.WantCaptureMouse);

		// Building the UI in the new frame.
		if (a_pUiBuild != nullptr)
		{
			a_pUiBuild();
		}
	}

	/// <summary>
	/// Initializes the ImGui ui framework.
	/// </summary>
	inline void Initialize()
	{
		// Reserve memory for ImGui's font textures.
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		Graphics::ReserveDescriptorHeapSlot(&cpuHandle, &gpuHandle);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui_ImplWin32_Init(Window::GetHandle());

		// Setting the initialization details for ImGui.
		ImGui_ImplDX12_InitInfo info{};
		info.CommandQueue = Graphics::CommandQueue.Get();
		info.Device = Graphics::Device.Get();
		info.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		info.LegacySingleSrvCpuDescriptor = cpuHandle;
		info.LegacySingleSrvGpuDescriptor = gpuHandle;
		info.NumFramesInFlight = Graphics::NumBackBuffers;
		info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		info.SrvDescriptorHeap = Graphics::CBVSRVDescriptorHeap.Get();
		ImGui_ImplDX12_Init(&info);
	}

	/// <summary>
	/// Renders the ImGui's UI frame.
	/// </summary>
	inline void Render()
	{
		// Ensuring that everything is in the proper state for ImGui.
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = Graphics::BackBuffers[Graphics::SwapChainIndex()].Get();
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		Graphics::CommandList->ResourceBarrier(1, &rb);

		// Providing ImGui the location of its font texture data.
		Graphics::CommandList->SetDescriptorHeaps(1, Graphics::CBVSRVDescriptorHeap.GetAddressOf());
		Graphics::CommandList->OMSetRenderTargets(1, &Graphics::RTVHandles[Graphics::SwapChainIndex()], true, 0);
		ImGui::Render();

		ImGui_ImplDX12_RenderDrawData(
			ImGui::GetDrawData(), 
			Graphics::CommandList.Get());

		// Transition back to the present state for the main program.
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		Graphics::CommandList->ResourceBarrier(1, &rb);
	}

	/// <summary>
	/// Cleans up ImGui and the memory it took up.
	/// </summary>
	inline void Shutdown()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
}

#endif //__INTERFACE_H_

