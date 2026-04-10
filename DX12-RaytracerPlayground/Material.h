#ifndef __MATERIAL_H_
#define __MATERIAL_H_

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

/// <summary>
/// Contains the indices for bindless texture mapping.
/// </summary>
struct TextureSet
{
	unsigned int AlbedoIndex    = (unsigned int) -1;
	unsigned int NormalIndex    = (unsigned int) -1;
	unsigned int MetallicIndex  = (unsigned int) -1;
	unsigned int RoughnessIndex = (unsigned int) -1;
	unsigned int EmissisveIndex = (unsigned int) -1;
};

/// <summary>
/// Defines the contents necessary for a material.
/// </summary>
class Material
{
private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	TextureSet m_textureSet;
	DirectX::XMFLOAT3 m_v3ColorTint;
	DirectX::XMFLOAT2 m_v2UVScale;
	DirectX::XMFLOAT2 m_v2UVOffset;

	float m_fRoughness;
	float m_fMetallic;

public:
	/// <summary>
	/// Initializes a new Material with the passed in data.
	/// </summary>
	Material(
		Microsoft::WRL::ComPtr<ID3D12PipelineState> a_pipelineState,
		TextureSet a_textureSet,
		DirectX::XMFLOAT3 a_v3ColorTint,
		DirectX::XMFLOAT2 a_v2UVScale,
		DirectX::XMFLOAT2 a_v2UVOffset,
		float a_fRoughness = 1.0f,
		float a_fMetallic = 0.0f);

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	TextureSet& GetTextureSet();
	DirectX::XMFLOAT3& GetColorTint();
	DirectX::XMFLOAT2& GetUVScale();
	DirectX::XMFLOAT2& GetUVOffset();

	float GetRoughness();
	float GetMetalness();
};

#endif //__MATERIAL_H_

