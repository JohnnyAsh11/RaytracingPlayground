#ifndef __ENTITY_H_
#define __ENTITY_H_

#include "Mesh.h"
#include "Transform.h"
#include "Material.h"

#include <memory>

/// <summary>
/// Contains the data for an entity in the scene, such as a mesh and transform.
/// </summary>
class Entity
{
private:
	std::shared_ptr<Mesh> m_pMesh;
	std::shared_ptr<Material> m_pMaterial;
	Transform m_transform;

public:
	/// <summary>
	/// Constructs the entity with the given mesh/material and a default transform.
	/// </summary>
	Entity(std::shared_ptr<Mesh> a_pMesh, std::shared_ptr<Material> a_pMaterial);

	/// <summary>
	/// Gets this Entity's transform.
	/// </summary>
	Transform& GetTransform();

	/// <summary>
	/// Gets this Entity's mesh.
	/// </summary>
	std::shared_ptr<Mesh> GetMesh();

	/// <summary>
	/// Gets the Entity's material.
	/// </summary>
	std::shared_ptr<Material> GetMaterial();
};

#endif //__ENTITY_H_

