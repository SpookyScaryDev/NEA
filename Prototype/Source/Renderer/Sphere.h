#pragma once

#include <Renderer/Object.h>
#include <Maths/Ray.h>

namespace Prototype {

class Sphere : Object {
public:
	                    Sphere(Vector3f position, float radius);
	virtual bool        Intersect(const Ray& ray) const override;

	float               radius;
};

}