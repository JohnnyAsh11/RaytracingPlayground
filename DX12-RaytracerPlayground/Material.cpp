#include "Material.h"

Material::Material(
	Microsoft::WRL::ComPtr<ID3D12PipelineState> a_pipelineState, 
	TextureSet a_textureSet,
	DirectX::XMFLOAT3 a_v3ColorTint,
	DirectX::XMFLOAT2 a_v2UVScale, 
	DirectX::XMFLOAT2 a_v2UVOffset,
	float a_fRoughness,
	float a_fMetallic,
	float a_fBrilliance)
{

	m_pipelineState = a_pipelineState;
	m_textureSet = a_textureSet;
	m_v3ColorTint = a_v3ColorTint;
	m_v2UVScale = a_v2UVScale;
	m_v2UVOffset = a_v2UVOffset;
	m_fMetallic = a_fMetallic;
	m_fRoughness = a_fRoughness;
	m_fBrilliance = a_fBrilliance;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState() { return m_pipelineState; }
TextureSet& Material::GetTextureSet() { return m_textureSet; }
DirectX::XMFLOAT3& Material::GetColorTint() { return m_v3ColorTint; }
DirectX::XMFLOAT2& Material::GetUVScale() { return m_v2UVScale; }
DirectX::XMFLOAT2& Material::GetUVOffset() { return m_v2UVOffset; }
float Material::GetBrilliance() { return m_fBrilliance; }
float Material::GetRoughness() { return m_fRoughness; }
float Material::GetMetalness() { return m_fMetallic; }

void Material::SetBrilliance(float a_fBrilliance)
{
	m_fBrilliance = a_fBrilliance;
}
