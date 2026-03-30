#ifndef __APPLICATION_H_
#define __APPLICATION_H_

#include "Camera.h"
#include "Entity.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

class Application
{
private:
	// Pipeline
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> emptyPso;

	// Geometry
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW ibView{};

	// Other graphics data
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};

	std::vector<std::shared_ptr<Entity>> m_lEntities;
	std::shared_ptr<Entity> m_pTorus;
	std::shared_ptr<Entity> m_pFloor;

	std::shared_ptr<Camera> m_pCamera;

public:
	Application();
	~Application();
	Application(const Application&) = delete; // Remove copy constructor
	Application& operator=(const Application&) = delete; // Remove copy-assignment operator

	void Update(float a_fDeltaTime, float a_fTotalTime);
	void Draw(float a_fDeltaTime, float a_fTotalTime);
	void OnResize();
};

#endif // __APPLICATION_H_

