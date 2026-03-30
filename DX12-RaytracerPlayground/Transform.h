#ifndef __TRANSFORM_H_
#define __TRANSFORM_H_

#include <DirectXMath.h>

/// <summary>
/// Controls the world matrix of objects in the world.
/// </summary>
class Transform
{
private:
	DirectX::XMFLOAT4X4 m_m4WorldMatrix;
	DirectX::XMFLOAT4X4 m_m4WorldInverseTranspose;

	DirectX::XMFLOAT3 m_v3Position;
	DirectX::XMFLOAT3 m_v3Scale;
	DirectX::XMFLOAT3 m_v3Rotation;

	bool m_bIsDirty;

public:
	Transform();

	void SetPosition(float a_fX, float a_fY, float a_fZ);
	void SetPosition(DirectX::XMFLOAT3 a_v3Position);

	void SetRotation(float a_fP, float a_fY, float a_fR);
	void SetRotation(DirectX::XMFLOAT3 a_v3Rotation);

	void SetScale(float a_fX, float a_fY, float a_fZ);
	void SetScale(DirectX::XMFLOAT3 a_v3Scale);

	void MoveAbsolute(float a_fX, float a_fY, float a_fZ);
	void MoveAbsolute(DirectX::XMFLOAT3 a_v3Offset);

	void MoveRelative(float a_fX, float a_fY, float a_fZ);
	void MoveRelative(DirectX::XMFLOAT3 a_v3Offset);

	void Rotate(float a_fP, float a_fY, float a_fR);
	void Rotate(DirectX::XMFLOAT3 a_v3Rotation);

	void Scale(float a_fX, float a_fY, float a_fZ);
	void Scale(DirectX::XMFLOAT3 a_v3Scale);

	DirectX::XMFLOAT3& GetPosition();
	DirectX::XMFLOAT3& GetRotation();
	DirectX::XMFLOAT3& GetScale();

	DirectX::XMFLOAT3 GetUp();
	DirectX::XMFLOAT3 GetRight();
	DirectX::XMFLOAT3 GetForward();

	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();

private:
	void CalculateMatrices(void);
};

#endif //__TRANSFORM_H_