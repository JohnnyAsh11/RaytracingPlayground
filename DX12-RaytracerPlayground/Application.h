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
	// Window resizing data.
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};
	Microsoft::WRL::ComPtr<ID3D12PipelineState> emptyPso;

	unsigned int m_uCubemapIndex;
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

