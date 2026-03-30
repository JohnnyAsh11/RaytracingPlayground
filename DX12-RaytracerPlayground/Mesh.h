#ifndef __MESH_H_
#define __MESH_H_

#include <d3d12.h>
#include <wrl/client.h>
#include "Vertex.h"

struct MeshRayTracingData
{
	D3D12_GPU_DESCRIPTOR_HANDLE IndexBufferSRV{};
	D3D12_GPU_DESCRIPTOR_HANDLE VertexBufferSRV{};
	Microsoft::WRL::ComPtr<ID3D12Resource> BLAS;
};

/// <summary>
/// Manages Vertex/Index Buffer objects for rendering to the window.
/// </summary>
class Mesh
{
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	int m_dIndexCount;
	int m_dVertexCount;

	MeshRayTracingData m_rayTracingData;

public:
	/// <summary>
	/// Constructs an instance of the Mesh class.
	/// </summary>
	/// <param name="a_pVertices">The vertices inside of this instance of a Mesh object.</param>
	/// <param name="a_dVertexCount">The quantity of vertices in the array.</param>
	/// <param name="a_pIndices">The indices inside of this instance of a Mesh object.</param>
	/// <param name="a_dIndexCount">The amount of indices in the array.</param>
	Mesh(Vertex* a_pVertices, int a_dVertexCount, unsigned int* a_pIndices, int a_dIndexCount);

	/// <summary>
	/// Loads in the vertices from an obj file.
	/// </summary>
	/// <param name="a_sFilepath">File path to the obj file.</param>
	Mesh(const char* a_sFilepath);

	#pragma region Rule of Three
	/// <summary>
	/// Destructs instances of the Mesh object.
	/// </summary>
	~Mesh();
	/// <summary>
	/// Copy constructor instantiates a copy of a passed in mesh object.
	/// </summary>
	/// <param name="a_pOther">The mesh object that this one is copying.</param>
	Mesh(const Mesh& a_pOther);
	/// <summary>
	/// Copy operator deep copies data from the assigned mesh object.
	/// </summary>
	/// <param name="a_pOther">The mesh that is having its data copied.</param>
	/// <returns>The new copy.</returns>
	Mesh& operator= (const Mesh& a_pOther);
	#pragma endregion

	// Accessors:
	/// <summary>
	/// Retrieves the Vertex Buffer ComPtr.
	/// </summary>
	/// <returns>The Vertex Buffer ptr.</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> GetVertexBuffer(void);

	/// <summary>
	/// Gets the Vertex Buffer View struct.
	/// </summary>
	D3D12_VERTEX_BUFFER_VIEW& GetVertexView();

	/// <summary>
	/// Gets the Index Buffer View struct.
	/// </summary>
	D3D12_INDEX_BUFFER_VIEW& GetIndexView();

	/// <summary>
	/// Retrieves the Index Buffer ComPtr.
	/// </summary>
	/// <returns>The Index Buffer ptr.</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> GetIndexBuffer(void);

	/// <summary>
	/// Retrieves the amount of indices in the Index Buffer.
	/// </summary>
	/// <returns>The amount of indices in the Index Buffer.</returns>
	int GetIndexCount(void);

	/// <summary>
	/// Retrieves the amount of vertices in the Vertex Buffer.
	/// </summary>
	/// <returns>The amount of vertices in the Vertex Buffer.</returns>
	int GetVertexCount(void);

	/// <summary>
	/// Gets the Ray Tracing data struct for this Mesh.
	/// </summary>
	const MeshRayTracingData& GetRayTracingData(void);

private:
	void CalculateTangents(
		Vertex* a_lVertices, 
		int a_dVertexCount,
		unsigned int* a_lIndices, 
		int a_dIndexCount);
};

#endif //__MESH_H_
