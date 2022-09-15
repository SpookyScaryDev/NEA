#pragma once

#include <string>
#include <Maths/Vector3f.h>
#include <Maths/Ray.h>
#include <Renderer/RayPayload.h>
#include <Renderer/Material.h>

namespace Prototype {

class Object {
public:
	                    Object(Vector3f position, Material material) : position(position), material(material){};
	virtual bool        Intersect(const Ray& ray, float min, float max, RayPayload& payload) = 0;
	Vector3f            position;
	Material            material;
	bool                show = true;
	int                 id;
	std::string         name;
};

}