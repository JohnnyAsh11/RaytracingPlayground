#include "Camera.h"
#include "Input.h"

// Including the M_PI macro
#define _USE_MATH_DEFINES
#include <math.h>

using namespace DirectX;

Camera::Camera(float a_fAspectRatio, XMFLOAT3 a_v3StartingPosition, float a_fFOV)
{
	m_fFOV = a_fFOV;
	m_tTransform = Transform();
	m_tTransform.SetPosition(a_v3StartingPosition);

	UpdateView();
	UpdateProjection(a_fAspectRatio);
}

DirectX::XMFLOAT4X4 Camera::GetView()
{
	return m_m4View;
}

DirectX::XMFLOAT4X4 Camera::GetProjection()
{
	return m_m4Projection;
}

Transform Camera::GetTransform()
{
	return m_tTransform;
}

void Camera::UpdateProjection(float a_fAspectRatio)
{
	// Creating the Projection matrix.
	XMMATRIX m4 = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(m_fFOV),				// FOV
		a_fAspectRatio,							// Aspect Ratio
		0.01f,									// Near Plane
		900.0f);								// Far Plane

	// Storing the projection matrix in the field.
	XMStoreFloat4x4(
		&m_m4Projection,
		m4);
}

void Camera::UpdateView()
{
	// Setting the up vector.
	XMFLOAT3 v3Up = m_tTransform.GetUp();
	XMVECTOR vUp = XMVectorSet(v3Up.x, v3Up.y, v3Up.z, 0.0f);

	// Setting the forward vector.
	XMFLOAT3 v3Forward = m_tTransform.GetForward();
	XMVECTOR vForward = XMVectorSet(v3Forward.x, v3Forward.y, v3Forward.z, 0.0f);

	// Creating the View matrix.
	XMMATRIX m4 = XMMatrixLookToLH(
		XMLoadFloat3(&m_tTransform.GetPosition()),		// Camera Position
		vForward,										// Camera Forward
		vUp);											// Camera Up

	// Storing the view matrix in the field.
	XMStoreFloat4x4(
		&m_m4View,
		m4);
}

void Camera::Update(float a_fDeltaTime)
{
	float fSpeed = 1.0f * a_fDeltaTime;

	// Forward and back movement.
	if (Input::KeyDown('W'))
	{
		m_tTransform.MoveRelative(0.0f, 0.0f, fSpeed);
	}
	if (Input::KeyDown('S'))
	{
		m_tTransform.MoveRelative(0.0f, 0.0f, -fSpeed);
	}

	// Left and right movement.
	if (Input::KeyDown('A'))
	{
		m_tTransform.MoveRelative(-fSpeed, 0.0f, 0.0f);

	}
	if (Input::KeyDown('D'))
	{
		m_tTransform.MoveRelative(fSpeed, 0.0f, 0.0f);
	}

	// Up and down movement.
	if (Input::KeyDown(VK_SPACE))
	{
		m_tTransform.MoveAbsolute(0.0f, fSpeed, 0.0f);
	}
	if (Input::KeyDown('X'))
	{
		m_tTransform.MoveAbsolute(0.0f, -fSpeed, 0.0f);
	}

	// Mouse input checking.
	if (Input::MouseLeftDown())
	{
		float fCursorDeltaX = Input::GetMouseXDelta() * 0.0025f;
		float fCursorDeltaY = Input::GetMouseYDelta() * 0.0025f;

		// Not very memory efficient but reduces operations.
		float fRotationX = m_tTransform.GetRotation().x + fCursorDeltaY;	// The future rotation.
		float fMin = (-90.0f * static_cast<float>(M_PI / 180.0f));			// -1/2PI
		float fMax = (90.0f * static_cast<float>(M_PI / 180.0f));			// 1/2PI
		float fPostClamp = max(fMin, min(fRotationX, fMax));

		// Checking if the clamp was done.  If it was, do not rotate around the X axis.
		if (fPostClamp == fMin || fPostClamp == fMax) fCursorDeltaY = 0.0f;

		// Rotating with the resulting values.
		m_tTransform.Rotate(fCursorDeltaY, fCursorDeltaX, 0.0f);
	}

	UpdateView();
}
