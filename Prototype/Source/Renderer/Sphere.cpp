#include "Sphere.h"

#include <math.h>

namespace Prototype {

Sphere::Sphere(Vector3f position, float radius) :
	radius(radius),
	Object(position) {}

bool Sphere::Intersect(const Ray& ray) const {
	float a = ray.GetDirection().Dot(ray.GetDirection());
	float b = 2.0 * ray.GetDirection().Dot(ray.GetOrigin() - position);
	float c = (ray.GetOrigin() - position).Dot((ray.GetOrigin() - position)) - pow(radius, 2.0);
	float d = -1;
	d = pow(b, 2) - 4.0 * a * c;
	return d >= 0;
}

}
