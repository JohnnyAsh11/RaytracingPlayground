#ifndef __CAMERA_H_
#define __CAMERA_H_

#include "Transform.h"

class Camera
{
private:
	float m_fFOV;
	Transform m_tTransform;
	DirectX::XMFLOAT4X4 m_m4View;
	DirectX::XMFLOAT4X4 m_m4Projection;

public:
	/// <summary>
	/// Constructs a camera with the passed in params.
	/// </summary>
	Camera(float a_fAspectRatio, DirectX::XMFLOAT3 a_v3StartingPosition, float a_fFOV);

	/// <summary>
	/// Gets the view matrix of the Camera.
	/// </summary>
	DirectX::XMFLOAT4X4 GetView();

	/// <summary>
	/// Gets the projection matrix of the Camera.
	/// </summary>
	DirectX::XMFLOAT4X4 GetProjection();

	/// <summary>
	/// Gets the Camera's transform.
	/// </summary>
	Transform GetTransform();

	/// <summary>
	/// Updates the projection matrix.
	/// </summary>
	void UpdateProjection(float a_fAspectRatio);

	/// <summary>
	/// Updates the View matrix.
	/// </summary>
	void UpdateView();

	/// <summary>
	/// Updates the position of the Camera.
	/// </summary>
	void Update(float a_fDeltaTime);
};

#endif //__CAMERA_H_
