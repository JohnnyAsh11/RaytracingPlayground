#ifndef __VERTEX_H_
#define __VERTEX_H_

#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT2 UV;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 Tangent;
};

#endif //__VERTEX_H_