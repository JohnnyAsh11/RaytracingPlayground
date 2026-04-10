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

Application::Application()
{
	srand(static_cast<unsigned int>(time(0)));

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
	std::shared_ptr<Mesh> quad = std::make_shared<Mesh>(FromExeDir("../../../Models/quad.graphics_obj").c_str());

	// Loading in all textures.
	TextureSet ornament{};
	ornament.AlbedoIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/ornament_albedo.png").c_str());
	ornament.NormalIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/ornament_normals.png").c_str());
	ornament.MetallicIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/ornament_metal.png").c_str());
	ornament.RoughnessIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/ornament_roughness.png").c_str());
	TextureSet cobblestone{};
	cobblestone.AlbedoIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/cobblestone_albedo.png").c_str());
	cobblestone.NormalIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/cobblestone_normals.png").c_str());
	cobblestone.MetallicIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/cobblestone_metal.png").c_str());
	cobblestone.RoughnessIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/cobblestone_roughness.png").c_str());
	TextureSet scratched{};
	scratched.AlbedoIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/scratched_albedo.png").c_str());
	scratched.NormalIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/scratched_normals.png").c_str());
	scratched.MetallicIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/scratched_metal.png").c_str());
	scratched.RoughnessIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/scratched_roughness.png").c_str());
	TextureSet floor{};
	floor.AlbedoIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/floor_albedo.png").c_str());
	floor.NormalIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/floor_normals.png").c_str());
	floor.MetallicIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/floor_metal.png").c_str());
	floor.RoughnessIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/floor_roughness.png").c_str());
	TextureSet lava{};
	lava.AlbedoIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/lava_albedo.png").c_str());
	lava.NormalIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/lava_normals.png").c_str());
	lava.RoughnessIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/lava_roughness.png").c_str());
	lava.EmissisveIndex = Graphics::LoadTexture(FromExeDir(L"../../../Textures/lava_emissive.png").c_str());
	
	// Creating a material with the PBR texture.
	m_lMaterials.push_back(std::make_shared<Material>(
		emptyPso,
		ornament,
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		XMFLOAT2(1.0f, 1.0f),
		XMFLOAT2(0.0f, 0.0f)));
	m_lMaterials.push_back(std::make_shared<Material>(
		emptyPso,
		cobblestone,
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		XMFLOAT2(1.0f, 1.0f),
		XMFLOAT2(0.0f, 0.0f)));
	m_lMaterials.push_back(std::make_shared<Material>(
		emptyPso,
		scratched,
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		XMFLOAT2(1.0f, 1.0f),
		XMFLOAT2(0.0f, 0.0f)));
	m_lMaterials.push_back(std::make_shared<Material>(
		emptyPso,
		floor,
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		XMFLOAT2(1.0f, 1.0f),
		XMFLOAT2(0.0f, 0.0f)));
	m_pLavaMaterial = std::make_shared<Material>(
		emptyPso,
		lava,
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		XMFLOAT2(1.0f, 1.0f),
		XMFLOAT2(0.0f, 0.0f),
		1.0f, 0.0f, 5.0f);
	m_lMaterials.push_back(m_pLavaMaterial);

	// Creating a material for the floor and mirror.
	TextureSet emptyTexture{};
	std::shared_ptr<Material> pFloorMat = std::make_shared<Material>(
		emptyPso,
		emptyTexture,
		XMFLOAT3(0.5f, 0.5f, 0.5f),
		XMFLOAT2(1.0f, 1.0f),
		XMFLOAT2(0.0f, 0.0f),
		1.0f, 0.0f);
	std::shared_ptr<Material> pMirrorMat = std::make_shared<Material>(
		emptyPso,
		emptyTexture,
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		XMFLOAT2(1.0f, 1.0f),
		XMFLOAT2(0.0f, 0.0f),
		0.2f, 1.0f);

	// Setting up the floor in the scene.
	m_pFloor = std::make_shared<Entity>(quad, pFloorMat);
	m_pFloor->GetTransform().Scale(20.0f, 20.0f, 20.0f);
	m_pFloor->GetTransform().MoveAbsolute(0.0f, -1.0f, 0.0f);
	m_lEntities.push_back(m_pFloor);

	// Setting up the floating Torus and Emissive Sphere.
	float scale = 1.5f;
	m_pTorus = std::make_shared<Entity>(torus, pMirrorMat);
	m_pTorus->GetTransform().Scale(scale, scale, scale);
	m_pTorus->GetTransform().MoveAbsolute(0.0f, 4.0f, 0.0f);
	m_lEntities.push_back(m_pTorus);
	m_lMaterials.push_back(pMirrorMat);

	const int MaxEntities = 15;
	float size = 0.5f;
	int distance = 0;
	for (int i = 0; i < MaxEntities; i++)
	{
		float rough = (float)1 - i % 2;
		float metal = (float)rand() / RAND_MAX;
		float r = (float)rand() / RAND_MAX;
		float g = (float)rand() / RAND_MAX;
		float b = (float)rand() / RAND_MAX;
		std::shared_ptr<Material> pMat;

		pMat = std::make_shared<Material>(
			emptyPso,
			emptyTexture,
			XMFLOAT3(r, g, b),
			XMFLOAT2(1.0f, 1.0f),
			XMFLOAT2(0.0f, 0.0f),
			rough,
			metal);

		std::shared_ptr<Entity> newEntity = std::make_shared<Entity>(sphere, pMat);

		distance += (size - 1.0f);
		newEntity->GetTransform().MoveAbsolute((i - (MaxEntities / 2)) + distance, (size - 1.0f), 0.0f);
		newEntity->GetTransform().Scale(size, size, size);

		m_lEntities.push_back(newEntity);
	}

	// Set some of the materials to be textures instead.
	int entCount = MaxEntities + 2;
	for (int i = 0; i < m_lMaterials.size(); i++)
	{
		m_lEntities[--entCount]->SetMaterial(m_lMaterials[i]);
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
	BuildUI(a_fDeltaTime);

	if (Interface::RotateMirror)
	{
		m_pTorus->GetTransform().Rotate(a_fDeltaTime, 0.0f, 0.0f);
	}

	if (Interface::IsMoving)
	{
		for (int i = 2; i < m_lEntities.size(); i++)
		{
			XMFLOAT3 position = m_lEntities[i]->GetTransform().GetPosition();
			XMFLOAT3 rotation = m_lEntities[i]->GetTransform().GetRotation();
			XMFLOAT3 scale = m_lEntities[i]->GetTransform().GetScale();

			float distance = 3.0f;
			float speed = 0.5f;
			position.z = (float)sin((a_fTotalTime / speed + i) / distance) * distance;
			rotation.x = position.z / (scale.x);

			m_lEntities[i]->GetTransform().SetPosition(position);
			m_lEntities[i]->GetTransform().SetRotation(rotation);
		}
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

void Application::BuildUI(float a_fDeltaTime)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = a_fDeltaTime;
	io.DisplaySize.x = (float)Window::GetWidth();
	io.DisplaySize.y = (float)Window::GetHeight();

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	Input::SetKeyboardCapture(io.WantCaptureKeyboard);
	Input::SetMouseCapture(io.WantCaptureMouse);

	ImGui::Begin("Debug");
	ImGui::Text("Frame rate: %f fps", ImGui::GetIO().Framerate);
	if (ImGui::TreeNode("General"))
	{
		ImGui::Checkbox("Create Sine Curve", &Interface::IsMoving);
		ImGui::Checkbox("Rotate Mirror", &Interface::RotateMirror);
		ImGui::TreePop();
	}
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
		ImGui::DragInt("Kernel Radius", &RayTracing::KernelRadius, 1, 1, 15);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Materials"))
	{
		float brilliance = m_pLavaMaterial->GetBrilliance();
		ImGui::DragFloat("Emissive Brilliance", &brilliance, 0.5f, 1.0f, 100.0f);
		m_pLavaMaterial->SetBrilliance(brilliance);

		ImGui::TreePop();
	}
	ImGui::End();
}
