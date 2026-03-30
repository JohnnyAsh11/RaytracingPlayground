#include "Entity.h"

Entity::Entity(std::shared_ptr<Mesh> a_pMesh, std::shared_ptr<Material> a_pMaterial)
{

	m_pMesh = a_pMesh;
	m_pMaterial = a_pMaterial;
	m_transform = Transform();

}

Transform& Entity::GetTransform() { return m_transform; }
std::shared_ptr<Mesh> Entity::GetMesh() { return m_pMesh; }
std::shared_ptr<Material> Entity::GetMaterial() { return m_pMaterial; }
