#include "Application.h"
#include "Interface.h"

#include "RayTracing.h"
#include "BufferStructs.h"
#include "FileHelper.h"

#include <DirectXMath.h>

#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;
void BuildImGui();

Application::Application()
{
	srand(time(0));

	// Initialize raytracing
	RayTracing::Initialize(
		Window::GetWidth(),
		Window::GetHeight(),
		FromExeDir(L"Raytracing.cso"));
	// Initialize ImGui.
	Interface::Initialize();

	m_pCamera = std::make_shared<Camera>(Window::GetAspectRatio(), XMFLOAT3(0.0f, 2.0f, -10.0f), 45.0f);
	m_uCubemapIndex = Graphics::LoadCubeMap(
		FromExeDir(L"../../../Cubemap/right.png").c_str(),
		FromExeDir(L"../../../Cubemap/left.png").c_str(),
		FromExeDir(L"../../../Cubemap/up.png").c_str(),
		FromExeDir(L"../../../Cubemap/down.png").c_str(),
		FromExeDir(L"../../../Cubemap/front.png").c_str(),
		FromExeDir(L"../../../Cubemap/back.png").c_str());

	// Loading in the models.
	std::shared_ptr<Mesh> cube = std::make_shared<Mesh>(FromExeDir("../../../Models/cube.graphics_obj").c_str());
	std::shared_ptr<Mesh> torus = std::make_shared<Mesh>(FromExeDir("../../../Models/torus.graphics_obj").c_str());
	std::shared_ptr<Mesh> sphere = std::make_shared<Mesh>(FromExeDir("../../../Models/sphere.graphics_obj").c_str());

	// Loading in a texture and creating an empty texture.
	TextureSet emptyTexture{};
	TextureSet cobblestone{};
	cobblestone.AlbedoIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/scratched_albedo.png").c_str());
	cobblestone.NormalIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/scratched_normals.png").c_str());
	cobblestone.MetallicIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/scratched_metal.png").c_str());
	cobblestone.RoughnessIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/scratched_roughness.png").c_str());
	
	// Creating a material with the PBR texture.
	std::shared_ptr<Material> pCobblestoneMat = std::make_shared<Material>(
		emptyPso,
		cobblestone,
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		XMFLOAT2(1.0f, 1.0f),
		XMFLOAT2(0.0f, 0.0f),
		1.0f,
		0.0f);

	// Creating a material for the floor.
	std::shared_ptr<Material> pFloorMat = std::make_shared<Material>(
		emptyPso,
		emptyTexture,
		XMFLOAT3(0.5f, 0.5f, 0.5f),
		XMFLOAT2(1.0f, 1.0f),
		XMFLOAT2(0.0f, 0.0f),
		1.0f, 0.0f);

	// Creating a basic mirror material.
	std::shared_ptr<Material> pMirrorMat = std::make_shared<Material>(
		emptyPso,
		emptyTexture,
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		XMFLOAT2(1.0f, 1.0f),
		XMFLOAT2(0.0f, 0.0f),
		0.2f, 1.0f);

	// Setting up the floor in the scene.
	m_pFloor = std::make_shared<Entity>(cube, pFloorMat);
	m_pFloor->GetTransform().Scale(20.0f, 10.0f, 30.0f);
	m_pFloor->GetTransform().MoveAbsolute(0.0f, -11.0f, 0.0f);
	m_lEntities.push_back(m_pFloor);

	// Setting up the floating Torus.
	m_pTorus = std::make_shared<Entity>(torus, pMirrorMat);
	m_pTorus->GetTransform().Scale(2.0f, 2.0f, 2.0f);
	m_pTorus->GetTransform().MoveAbsolute(0.0f, 4.0f, 0.0f);
	m_lEntities.push_back(m_pTorus);

	const int MaxEntities = 15;
	for (int i = 0; i < MaxEntities; i++)
	{
		float rough = (float)1 - i % 2;
		float metal = (float)rand() / RAND_MAX;
		float size = (float)rand() / RAND_MAX;
		if (size < 0.2f) size = 0.2f;
		if (size > 0.8f) size = 0.6f;
		float r = (float)rand() / RAND_MAX;
		float g = (float)rand() / RAND_MAX;
		float b = (float)rand() / RAND_MAX;
		bool isTextureMat = r > 0.7f;
		std::shared_ptr<Material> pMat;
		if (isTextureMat)
		{
			pMat = pCobblestoneMat;
		}
		else
		{
			pMat = std::make_shared<Material>(
				emptyPso,
				emptyTexture,
				XMFLOAT3(r, g, b),
				XMFLOAT2(1.0f, 1.0f),
				XMFLOAT2(0.0f, 0.0f),
				rough,
				metal);
		}

		std::shared_ptr<Entity> newEntity = std::make_shared<Entity>(sphere, pMat);
		float radius = 5.0f;
		float theta = ((float)rand() / RAND_MAX) * (2 * 3.14159265359f);
		float x = (float)cos(theta) * radius;
		float z = (float)sin(theta) * radius;
		newEntity->GetTransform().MoveAbsolute(x, (size - 1.0f), z);
		newEntity->GetTransform().Scale(size, size, size);

		m_lEntities.push_back(newEntity);
	}

	RayTracing::CreateEntityDataBuffer(m_lEntities);
	// Once we have all of the BLASs ready, we can make a TLAS
	RayTracing::CreateTopLevelAccelerationStructureForScene(m_lEntities);
	// Finalize any initialization and wait for the GPU
	// before proceeding to the game loop
	Graphics::CloseAndExecuteCommandList();
	Graphics::WaitForGPU();
	Graphics::ResetAllocatorAndCommandList();
}

Application::~Application()
{
	Graphics::WaitForGPU();
	Interface::Shutdown();
}

void Application::Update(float a_fDeltaTime, float a_fTotalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::KeyDown(VK_ESCAPE))
	{
		Window::Shutdown();
	}
	Interface::SetFrame(BuildImGui, a_fDeltaTime);

	m_pTorus->GetTransform().Rotate(a_fDeltaTime, 0.0f, 0.0f);
	for (int i = 2; i < m_lEntities.size(); i++)
	{
		XMFLOAT3 position = m_lEntities[i]->GetTransform().GetPosition();
		XMFLOAT3 rotation = m_lEntities[i]->GetTransform().GetRotation();
		XMFLOAT3 scale = m_lEntities[i]->GetTransform().GetScale();

		float distance = 3.0f;
		float speed = 0.5f;
		if (i % 2)
		{
			position.x = (float)sin((a_fTotalTime / speed + i) / distance) * distance;
			rotation.z = -position.x / (scale.x);
		}
		else
		{
			position.z = (float)sin((a_fTotalTime / speed + i) / distance) * distance;
			rotation.x = position.z / (scale.x);
		}

		m_lEntities[i]->GetTransform().SetPosition(position);
		m_lEntities[i]->GetTransform().SetRotation(rotation);
	}

	// Update the camera.
	m_pCamera->Update(a_fDeltaTime);
}

void Application::Draw(float a_fDeltaTime, float a_fTotalTime)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer =
		Graphics::BackBuffers[Graphics::SwapChainIndex()];

	// Creating the TLAS for raytracing and dispatching rays.
	RayTracing::CreateTopLevelAccelerationStructureForScene(m_lEntities);
	RayTracing::Raytrace(m_pCamera, currentBackBuffer, m_uCubemapIndex);

	// Rendering the interface for the application.
	Interface::Render();

	// Closing the command list and finishing this draw call.
	Graphics::CloseAndExecuteCommandList();

	// Present the current back buffer and increment to the next.
	bool vsync = Graphics::VsyncState();
	Graphics::SwapChain->Present(
		vsync ? 1 : 0,
		vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);
	Graphics::AdvanceSwapChainIndex();
	// Also reset the command list/allocator for upcoming frames.
	Graphics::ResetAllocatorAndCommandList();
}

void Application::OnResize()
{
	// Setting up the viewport.
	viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float)Window::GetWidth();
	viewport.Height = (float)Window::GetHeight();
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Not doing anything special with the scissor rect.
	scissorRect = {};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = Window::GetWidth();
	scissorRect.bottom = Window::GetHeight();

	// Updating the projection matrix.
	m_pCamera->UpdateProjection(Window::GetAspectRatio());
	// Resize raytracing output texture.
	RayTracing::ResizeOutputUAV(Window::GetWidth(), Window::GetHeight());
}

void BuildImGui()
{
	ImGui::Begin("Debug");
	ImGui::Text("Frame rate: %f fps", ImGui::GetIO().Framerate);
	if (ImGui::TreeNode("Path Tracing"))
	{
		ImGui::DragInt("Recursion Depth", &RayTracing::RecursionDepth, 1, 1, 20);
		ImGui::DragInt("Rays Per Pixel", &RayTracing::RaysPerPixel, 1, 1, 50);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Bilateral Filtering"))
	{
		ImGui::Checkbox("Bilateral Filtering", &RayTracing::m_bFilterOn);
		ImGui::DragFloat("Sigma Spatial", &RayTracing::SigmaSpatial, 0.5f, 1.0f, 15.0f);
		ImGui::DragFloat("Sigma Color", &RayTracing::SigmaColor, 0.1f, 0.1f, 5.0f);
		ImGui::DragInt("Kernel Radius", &RayTracing::KernelRadius, 1.0f, 1.0f, 15.0f);
		ImGui::TreePop();
	}
	ImGui::End();
}
