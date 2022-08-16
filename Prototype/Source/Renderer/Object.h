#pragma once

#include <Maths/Vector3f.h>
#include <Maths/Ray.h>

namespace Prototype {

class Object {
public:
	                    Object(Vector3f position) : position(position) {};
	virtual bool        Intersect(const Ray& ray) const = 0;
	Vector3f            position;
};

}