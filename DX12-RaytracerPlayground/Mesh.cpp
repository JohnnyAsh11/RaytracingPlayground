#include "Mesh.h"
#include "Graphics.h"
#include "RayTracing.h"

#include <vector>
#include <fstream>

using namespace DirectX;

Mesh::Mesh(Vertex* a_pVertices, int a_dVertexCount, unsigned int* a_pIndices, int a_dIndexCount)
{
	// Saving the passed in values to the member fields.
	m_pVertexBuffer = nullptr;
	m_pIndexBuffer = nullptr;

	m_dVertexCount = a_dVertexCount;
	m_dIndexCount = a_dIndexCount;

	// Calculating vertex tangents.
	CalculateTangents(a_pVertices, a_dVertexCount, a_pIndices, a_dIndexCount);

	// Creating the static buffer resources.
	m_pVertexBuffer = Graphics::CreateStaticBuffer(sizeof(Vertex), m_dVertexCount, a_pVertices);
	m_pIndexBuffer = Graphics::CreateStaticBuffer(sizeof(unsigned int), m_dIndexCount, a_pIndices);

	// Set up the views
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = sizeof(Vertex) * m_dVertexCount;
	m_vertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();

	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = sizeof(unsigned int) * m_dIndexCount;
	m_indexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();

	// Create the raytracing acceleration structure for this mesh
	m_rayTracingData = RayTracing::CreateBottomLevelAccelerationStructureForMesh(this);
}

Mesh::Mesh(const char* a_sFilepath)
{
	// Author: Chris Cascioli
	// Purpose: Basic .OBJ 3D model loading, supporting positions, uvs and normals
	
#pragma region File Loading
	// File input object
	std::ifstream obj(a_sFilepath);

	// Check for successful open
	if (!obj.is_open())
		throw std::invalid_argument("Error opening file: Invalid file path or file is inaccessible");

	// Variables used while reading the file
	std::vector<XMFLOAT3> positions;		// Positions from the file
	std::vector<XMFLOAT3> normals;			// Normals from the file
	std::vector<XMFLOAT2> uvs;				// UVs from the file
	std::vector<Vertex> verts;				// Verts we're assembling
	std::vector<unsigned int> indices;		// Indices of these verts
	int vertCounter = 0;					// Count of vertices
	int indexCounter = 0;					// Count of indices
	char chars[100];						// String for line reading

	// Still have data left?
	while (obj.good())
	{
		// Get the line (100 characters should be more than enough)
		obj.getline(chars, 100);

		// Check the type of line
		if (chars[0] == 'v' && chars[1] == 'n')
		{
			// Read the 3 numbers directly into an XMFLOAT3
			XMFLOAT3 norm;
			sscanf_s(
				chars,
				"vn %f %f %f",
				&norm.x, &norm.y, &norm.z);

			// Add to the list of normals
			normals.push_back(norm);
		}
		else if (chars[0] == 'v' && chars[1] == 't')
		{
			// Read the 2 numbers directly into an XMFLOAT2
			XMFLOAT2 uv;
			sscanf_s(
				chars,
				"vt %f %f",
				&uv.x, &uv.y);

			// Add to the list of uv's
			uvs.push_back(uv);
		}
		else if (chars[0] == 'v')
		{
			// Read the 3 numbers directly into an XMFLOAT3
			XMFLOAT3 pos;
			sscanf_s(
				chars,
				"v %f %f %f",
				&pos.x, &pos.y, &pos.z);

			// Add to the positions
			positions.push_back(pos);
		}
		else if (chars[0] == 'f')
		{
			// Read the face indices into an array
			// NOTE: This assumes the given obj file contains
			//  vertex positions, uv coordinates AND normals.
			unsigned int i[12];
			int numbersRead = sscanf_s(
				chars,
				"f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
				&i[0], &i[1], &i[2],
				&i[3], &i[4], &i[5],
				&i[6], &i[7], &i[8],
				&i[9], &i[10], &i[11]);

			// If we only got the first number, chances are the OBJ
			// file has no UV coordinates.  This isn't great, but we
			// still want to load the model without crashing, so we
			// need to re-read a different pattern (in which we assume
			// there are no UVs denoted for any of the vertices)
			if (numbersRead == 1)
			{
				// Re-read with a different pattern
				numbersRead = sscanf_s(
					chars,
					"f %d//%d %d//%d %d//%d %d//%d",
					&i[0], &i[2],
					&i[3], &i[5],
					&i[6], &i[8],
					&i[9], &i[11]);

				// The following indices are where the UVs should 
				// have been, so give them a valid value
				i[1] = 1;
				i[4] = 1;
				i[7] = 1;
				i[10] = 1;

				// If we have no UVs, create a single UV coordinate
				// that will be used for all vertices
				if (uvs.size() == 0)
					uvs.push_back(XMFLOAT2(0, 0));
			}

			// - Create the verts by looking up
			//    corresponding data from vectors
			// - OBJ File indices are 1-based, so
			//    they need to be adusted
			Vertex v1;
			v1.Position = positions[i[0] - 1];
			v1.UV = uvs[i[1] - 1];
			v1.Normal = normals[i[2] - 1];

			Vertex v2;
			v2.Position = positions[i[3] - 1];
			v2.UV = uvs[i[4] - 1];
			v2.Normal = normals[i[5] - 1];

			Vertex v3;
			v3.Position = positions[i[6] - 1];
			v3.UV = uvs[i[7] - 1];
			v3.Normal = normals[i[8] - 1];

			// The model is most likely in a right-handed space,
			// especially if it came from Maya.  We want to convert
			// to a left-handed space for DirectX.  This means we 
			// need to:
			//  - Invert the Z position
			//  - Invert the normal's Z
			//  - Flip the winding order
			// We also need to flip the UV coordinate since DirectX
			// defines (0,0) as the top left of the texture, and many
			// 3D modeling packages use the bottom left as (0,0)

			// Flip the UV's since they're probably "upside down"
			v1.UV.y = 1.0f - v1.UV.y;
			v2.UV.y = 1.0f - v2.UV.y;
			v3.UV.y = 1.0f - v3.UV.y;

			// Flip Z (LH vs. RH)
			v1.Position.z *= -1.0f;
			v2.Position.z *= -1.0f;
			v3.Position.z *= -1.0f;

			// Flip normal's Z
			v1.Normal.z *= -1.0f;
			v2.Normal.z *= -1.0f;
			v3.Normal.z *= -1.0f;

			// Add the verts to the vector (flipping the winding order)
			verts.push_back(v1);
			verts.push_back(v3);
			verts.push_back(v2);
			vertCounter += 3;

			// Add three more indices
			indices.push_back(indexCounter); indexCounter += 1;
			indices.push_back(indexCounter); indexCounter += 1;
			indices.push_back(indexCounter); indexCounter += 1;

			// Was there a 4th face?
			// - 12 numbers read means 4 faces WITH uv's
			// - 8 numbers read means 4 faces WITHOUT uv's
			if (numbersRead == 12 || numbersRead == 8)
			{
				// Make the last vertex
				Vertex v4;
				v4.Position = positions[i[9] - 1];
				v4.UV = uvs[i[10] - 1];
				v4.Normal = normals[i[11] - 1];

				// Flip the UV, Z pos and normal's Z
				v4.UV.y = 1.0f - v4.UV.y;
				v4.Position.z *= -1.0f;
				v4.Normal.z *= -1.0f;

				// Add a whole triangle (flipping the winding order)
				verts.push_back(v1);
				verts.push_back(v4);
				verts.push_back(v3);
				vertCounter += 3;

				// Add three more indices
				indices.push_back(indexCounter); indexCounter += 1;
				indices.push_back(indexCounter); indexCounter += 1;
				indices.push_back(indexCounter); indexCounter += 1;
			}
		}
	}
#pragma endregion

	// Close the file and create the actual buffers
	obj.close();

	m_dVertexCount = vertCounter;
	m_dIndexCount = indexCounter;

	// Calculate vertex tangents.
	CalculateTangents(verts.data(), m_dVertexCount, indices.data(), m_dIndexCount);

	// Creating the static buffer resources.
	m_pVertexBuffer = Graphics::CreateStaticBuffer(sizeof(Vertex), m_dVertexCount, verts.data());
	m_pIndexBuffer = Graphics::CreateStaticBuffer(sizeof(unsigned int), m_dIndexCount, indices.data());

	// Set up the views
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = sizeof(Vertex) * m_dVertexCount;
	m_vertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();

	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = sizeof(unsigned int) * m_dIndexCount;
	m_indexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();

	// Create the raytracing acceleration structure for this mesh
	m_rayTracingData = RayTracing::CreateBottomLevelAccelerationStructureForMesh(this);
}

Mesh::~Mesh() 
{
	// Freeing memory.
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();
}
Mesh::Mesh(const Mesh& a_pOther) 
{
	// Setting values.
	m_pVertexBuffer = a_pOther.m_pVertexBuffer;
	m_dVertexCount = a_pOther.m_dVertexCount;
	m_pIndexBuffer = a_pOther.m_pIndexBuffer;
	m_dIndexCount = a_pOther.m_dIndexCount;
	m_vertexBufferView = a_pOther.m_vertexBufferView;
	m_indexBufferView = a_pOther.m_indexBufferView;

	// Create the raytracing acceleration structure for this mesh
	m_rayTracingData = RayTracing::CreateBottomLevelAccelerationStructureForMesh(this);
}
Mesh& Mesh::operator=(const Mesh& a_pOther)
{
	// Freeing the memory in the ComPtrs.
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();

	// Setting values.
	m_pVertexBuffer = a_pOther.m_pVertexBuffer;
	m_dVertexCount = a_pOther.m_dVertexCount;
	m_pIndexBuffer = a_pOther.m_pIndexBuffer;
	m_dIndexCount = a_pOther.m_dIndexCount;

	// Create the raytracing acceleration structure for this mesh
	m_rayTracingData = RayTracing::CreateBottomLevelAccelerationStructureForMesh(this);

	return *this;
}

#pragma region Accessors
Microsoft::WRL::ComPtr<ID3D12Resource> Mesh::GetVertexBuffer(void)
{
	return m_pVertexBuffer;
}
D3D12_VERTEX_BUFFER_VIEW& Mesh::GetVertexView()
{
	return m_vertexBufferView;
}
D3D12_INDEX_BUFFER_VIEW& Mesh::GetIndexView()
{
	return m_indexBufferView;
}
Microsoft::WRL::ComPtr<ID3D12Resource> Mesh::GetIndexBuffer(void)
{
	return m_pIndexBuffer;
}
int Mesh::GetIndexCount(void)
{
	return m_dIndexCount;
}
int Mesh::GetVertexCount(void)
{
	return m_dVertexCount;
}
const MeshRayTracingData& Mesh::GetRayTracingData(void)
{
	return m_rayTracingData;
}
#pragma endregion

// --------------------------------------------------------
// Calculates the tangents of the vertices in a mesh
// - Code originally adapted from: http://www.terathon.com/code/tangent.html
// - Updated version found here: http://foundationsofgameenginedev.com/FGED2-sample.pdf
// - See listing 7.4 in section 7.5 (page 9 of the PDF)
//
// - Note: For this code to work, your Vertex format must
// contain an XMFLOAT3 called Tangent
//
// - Be sure to call this BEFORE creating your D3D vertex/index buffers
// --------------------------------------------------------
void Mesh::CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices)
{
	// Reset tangents
	for (int i = 0; i < numVerts; i++)
	{
		verts[i].Tangent = XMFLOAT3(0, 0, 0);
	}
	// Calculate tangents one whole triangle at a time
	for (int i = 0; i < numIndices;)
	{
		// Grab indices and vertices of first triangle
		unsigned int i1 = indices[i++];
		unsigned int i2 = indices[i++];
		unsigned int i3 = indices[i++];
		Vertex* v1 = &verts[i1];
		Vertex* v2 = &verts[i2];
		Vertex* v3 = &verts[i3];
		// Calculate vectors relative to triangle positions
		float x1 = v2->Position.x - v1->Position.x;
		float y1 = v2->Position.y - v1->Position.y;
		float z1 = v2->Position.z - v1->Position.z;
		float x2 = v3->Position.x - v1->Position.x;
		float y2 = v3->Position.y - v1->Position.y;
		float z2 = v3->Position.z - v1->Position.z;
		// Do the same for vectors relative to triangle uv's
		float s1 = v2->UV.x - v1->UV.x;
		float t1 = v2->UV.y - v1->UV.y;
		float s2 = v3->UV.x - v1->UV.x;
		float t2 = v3->UV.y - v1->UV.y;
		// Create vectors for tangent calculation
		float r = 1.0f / (s1 * t2 - s2 * t1);
		float tx = (t2 * x1 - t1 * x2) * r;
		float ty = (t2 * y1 - t1 * y2) * r;
		float tz = (t2 * z1 - t1 * z2) * r;
		// Adjust tangents of each vert of the triangle
		v1->Tangent.x += tx;
		v1->Tangent.y += ty;
		v1->Tangent.z += tz;
		v2->Tangent.x += tx;
		v2->Tangent.y += ty;
		v2->Tangent.z += tz;
		v3->Tangent.x += tx;
		v3->Tangent.y += ty;
		v3->Tangent.z += tz;
	}
	// Ensure all of the tangents are orthogonal to the normals
	for (int i = 0; i < numVerts; i++)
	{
		// Grab the two vectors
		XMVECTOR normal = XMLoadFloat3(&verts[i].Normal);
		XMVECTOR tangent = XMLoadFloat3(&verts[i].Tangent);
		// Use Gram-Schmidt orthonormalize to ensure
		// the normal and tangent are exactly 90 degrees apart
		tangent = XMVector3Normalize(
			tangent - normal * XMVector3Dot(normal, tangent));
		// Store the tangent
		XMStoreFloat3(&verts[i].Tangent, tangent);
	}
}