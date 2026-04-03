#ifndef __VERTEX_H_
#define __VERTEX_H_

#include <DirectXMath.h>

/// <summary>
/// Defines a single vertex within a Mesh object.
/// </summary>
struct Vertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT2 UV;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 Tangent;
};

#endif //__VERTEX_H_