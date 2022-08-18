#pragma once

#include <Renderer/Object.h>
#include <Maths/Ray.h>
#include <Renderer/Material.h>

namespace Prototype {

class Sphere : Object {
public:
	                    Sphere(Vector3f position, float radius, Material material);
	virtual bool        Intersect(const Ray& ray, float min, float max, RayPayload& payload) override;

	float               radius;
};

}