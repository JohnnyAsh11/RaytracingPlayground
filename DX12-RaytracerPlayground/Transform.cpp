#include "Transform.h"

using namespace DirectX;

Transform::Transform() :
    m_v3Position(0.0f, 0.0f, 0.0f),
    m_v3Rotation(0.0f, 0.0f, 0.0f),
    m_v3Scale(1.0f, 1.0f, 1.0f),
    m_bIsDirty(false)
{
    XMStoreFloat4x4(&m_m4WorldMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&m_m4WorldInverseTranspose, XMMatrixIdentity());
}

void Transform::SetPosition(float a_fX, float a_fY, float a_fZ)
{
    m_v3Position.x = a_fX;
    m_v3Position.y = a_fY;
    m_v3Position.z = a_fZ;

    m_bIsDirty = true;
}
void Transform::SetPosition(DirectX::XMFLOAT3 a_v3Position)
{
    m_v3Position.x = a_v3Position.x;
    m_v3Position.y = a_v3Position.y;
    m_v3Position.z = a_v3Position.z;

    m_bIsDirty = true;
}
void Transform::SetRotation(float a_fP, float a_fY, float a_fR)
{
    m_v3Rotation.x = a_fP;
    m_v3Rotation.y = a_fY;
    m_v3Rotation.z = a_fR;

    m_bIsDirty = true;
}
void Transform::SetRotation(DirectX::XMFLOAT3 a_v3Rotation)
{
    m_v3Rotation.x = a_v3Rotation.x;
    m_v3Rotation.y = a_v3Rotation.y;
    m_v3Rotation.z = a_v3Rotation.z;

    m_bIsDirty = true;
}
void Transform::SetScale(float a_fX, float a_fY, float a_fZ)
{
    m_v3Scale.x = a_fX;
    m_v3Scale.y = a_fY;
    m_v3Scale.z = a_fZ;

    m_bIsDirty = true;
}
void Transform::SetScale(DirectX::XMFLOAT3 a_v3Scale)
{
    m_v3Scale.x = a_v3Scale.x;
    m_v3Scale.y = a_v3Scale.y;
    m_v3Scale.z = a_v3Scale.z;

    m_bIsDirty = true;
}

void Transform::MoveAbsolute(float a_fX, float a_fY, float a_fZ)
{
    // Using the super optimized math functions to move the position.
    XMStoreFloat3(
        &m_v3Position,
        XMLoadFloat3(&m_v3Position) + XMVectorSet(a_fX, a_fY, a_fZ, 0.0f)
    );

    m_bIsDirty = true;
}
void Transform::MoveAbsolute(DirectX::XMFLOAT3 a_v3Offset)
{
    // Using the super optimized math functions to move the position.
    XMStoreFloat3(
        &m_v3Position,
        XMLoadFloat3(&m_v3Position) + XMVectorSet(a_v3Offset.x, a_v3Offset.y, a_v3Offset.z, 0.0f)
    );

    m_bIsDirty = true;
}
void Transform::MoveRelative(float a_fX, float a_fY, float a_fZ)
{
    // Creating the offset and quaternion vectors.
    XMFLOAT3 v3 = XMFLOAT3(a_fX, a_fY, a_fZ);
    XMVECTOR vOffset = XMLoadFloat3(&v3);
    XMVECTOR vQuat = XMQuaternionRotationRollPitchYaw(
        m_v3Rotation.x,     // Roll
        m_v3Rotation.y,     // Pitch
        m_v3Rotation.z);    // Yaw

    // Getting the resulting vector that we will actually move by.
    XMVECTOR vResult = XMVector3Rotate(vOffset, vQuat);

    // Adding that to the position and saving the result as the new position.
    XMStoreFloat3(
        &m_v3Position,
        XMLoadFloat3(&m_v3Position) + vResult
    );
}
void Transform::MoveRelative(DirectX::XMFLOAT3 a_v3Offset)
{
    // Creating the offset and quaternion vectors.
    XMVECTOR vOffset = XMLoadFloat3(&a_v3Offset);
    XMVECTOR vQuat = XMQuaternionRotationRollPitchYaw(
        m_v3Rotation.x,     // Roll
        m_v3Rotation.y,     // Pitch
        m_v3Rotation.z);    // Yaw

    // Getting the resulting vector that we will actually move by.
    XMVECTOR vResult = XMVector3Rotate(vOffset, vQuat);

    // Adding that to the position and saving the result as the new position.
    XMStoreFloat3(
        &m_v3Position,
        XMLoadFloat3(&m_v3Position) + vResult
    );
}
void Transform::Rotate(float a_fP, float a_fY, float a_fR)
{
    XMStoreFloat3(
        &m_v3Rotation,
        XMLoadFloat3(&m_v3Rotation) + XMVectorSet(a_fP, a_fY, a_fR, 1.0f)
    );

    m_bIsDirty = true;
}
void Transform::Rotate(DirectX::XMFLOAT3 a_v3Rotation)
{
    XMStoreFloat3(
        &m_v3Rotation,
        XMLoadFloat3(&m_v3Rotation) + XMVectorSet(a_v3Rotation.x, a_v3Rotation.y, a_v3Rotation.z, 1.0f)
    );

    m_bIsDirty = true;
}
void Transform::Scale(float a_fX, float a_fY, float a_fZ)
{
    XMStoreFloat3(
        &m_v3Scale,
        XMLoadFloat3(&m_v3Scale) * XMVectorSet(a_fX, a_fY, a_fY, 1.0f)
    );

    m_bIsDirty = true;
}
void Transform::Scale(DirectX::XMFLOAT3 a_v3Scale)
{
    XMStoreFloat3(
        &m_v3Scale,
        XMLoadFloat3(&m_v3Scale) * XMVectorSet(a_v3Scale.x, a_v3Scale.y, a_v3Scale.z, 1.0f)
    );

    m_bIsDirty = true;
}

DirectX::XMFLOAT3& Transform::GetPosition()
{
    // Assume that when returning the position that the data will be altered.
    m_bIsDirty = true;
    return m_v3Position;
}
DirectX::XMFLOAT3& Transform::GetRotation()
{
    // Assume that when returning the position that the data will be altered.
    m_bIsDirty = true;
    return m_v3Rotation;
}
DirectX::XMFLOAT3& Transform::GetScale()
{
    // Assume that when returning the position that the data will be altered.
    m_bIsDirty = true;
    return m_v3Scale;
}

DirectX::XMFLOAT3 Transform::GetUp()
{
    // Temporarily use this result vector for the direction
    XMFLOAT3 v3Result = XMFLOAT3(0.0f, 1.0f, 0.0f);

    // Creating the offset and quaternion vectors.
    XMVECTOR vWorldUp = XMLoadFloat3(&v3Result);
    XMVECTOR vOrientation = XMQuaternionRotationRollPitchYaw(
        m_v3Rotation.x,     // Roll
        m_v3Rotation.y,     // Pitch
        m_v3Rotation.z);    // Yaw

    XMStoreFloat3(
        &v3Result,
        XMVector3Rotate(vWorldUp, vOrientation)
    );

    return v3Result;
}
DirectX::XMFLOAT3 Transform::GetRight()
{
    // Temporarily use this result vector for the direction
    XMFLOAT3 v3Result = XMFLOAT3(1.0f, 0.0f, 0.0f);

    // Creating the offset and quaternion vectors.
    XMVECTOR vWorldUp = XMLoadFloat3(&v3Result);
    XMVECTOR vOrientation = XMQuaternionRotationRollPitchYaw(
        m_v3Rotation.x,     // Roll
        m_v3Rotation.y,     // Pitch
        m_v3Rotation.z);    // Yaw

    XMStoreFloat3(
        &v3Result,
        XMVector3Rotate(vWorldUp, vOrientation)
    );

    return v3Result;
}
DirectX::XMFLOAT3 Transform::GetForward()
{
    // Temporarily use this result vector for the direction
    XMFLOAT3 v3Result = XMFLOAT3(0.0f, 0.0f, 1.0f);

    // Creating the offset and quaternion vectors.
    XMVECTOR vWorldUp = XMLoadFloat3(&v3Result);
    XMVECTOR vOrientation = XMQuaternionRotationRollPitchYaw(
        m_v3Rotation.x,     // Roll
        m_v3Rotation.y,     // Pitch
        m_v3Rotation.z);    // Yaw

    XMStoreFloat3(
        &v3Result,
        XMVector3Rotate(vWorldUp, vOrientation)
    );

    return v3Result;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
    if (!m_bIsDirty) return m_m4WorldMatrix;

    CalculateMatrices();
    
    // Returning the resulting value.
    return m_m4WorldMatrix;
}
DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
    if (!m_bIsDirty) return m_m4WorldInverseTranspose;

    CalculateMatrices();

    // Returning the resulting value.
    return m_m4WorldInverseTranspose;
}

void Transform::CalculateMatrices(void)
{
    m_bIsDirty = false;

    // Calculating matrices based on the rotation/scale/translation vectors.
    XMMATRIX trMatrix = XMMatrixTranslation(m_v3Position.x, m_v3Position.y, m_v3Position.z);
    XMMATRIX roMatrix = XMMatrixRotationRollPitchYaw(m_v3Rotation.x, m_v3Rotation.y, m_v3Rotation.z);
    XMMATRIX scMatrix = XMMatrixScaling(m_v3Scale.x, m_v3Scale.y, m_v3Scale.z);

    // Calculating the world matrix.
    XMMATRIX world = scMatrix * roMatrix * trMatrix;

    // Storing the product of those matrices in the world matrix.
    XMStoreFloat4x4(&m_m4WorldMatrix, world);
    XMStoreFloat4x4(&m_m4WorldInverseTranspose,
        XMMatrixInverse(0, XMMatrixTranspose(world))
    );
}
