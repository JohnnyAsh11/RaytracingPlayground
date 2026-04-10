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
	// General application variables.
	Microsoft::WRL::ComPtr<ID3D12PipelineState> emptyPso;

	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};

	unsigned int m_uCubemapIndex;
	std::vector<std::shared_ptr<Entity>> m_lEntities;
	std::shared_ptr<Entity> m_pTorus;
	std::shared_ptr<Entity> m_pLavaSphere;
	std::shared_ptr<Entity> m_pFloor;
	std::shared_ptr<Camera> m_pCamera;

public:
	/// <summary>
	/// Initializes the application, creating the camera, entities, interface and much more.
	/// </summary>
	Application();

	/// <summary>
	/// Frees up the memory used by the application.
	/// </summary>
	~Application();

	// There is no need to copy or assign the application.  Doing so would cause issues with graphics resources.
	Application(const Application&) = delete;			 // Remove copy constructor
	Application& operator=(const Application&) = delete; // Remove copy-assignment operator

	/// <summary>
	/// Updates the application's containing entities, camera, and interface logic.
	/// </summary>
	void Update(float a_fDeltaTime, float a_fTotalTime);

	/// <summary>
	/// Renders everything in the application.  Does so primarily through raytracing.
	/// </summary>
	void Draw(float a_fDeltaTime, float a_fTotalTime);

	/// <summary>
	/// Resizes the application on window resizing.
	/// </summary>
	void OnResize();
};

#endif // __APPLICATION_H_

