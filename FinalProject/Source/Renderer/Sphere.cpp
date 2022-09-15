#include "Sphere.h"

#include <math.h>
#include <Renderer/Material.h>

namespace Prototype {

Sphere::Sphere(Vector3f position, float radius, Material material) :
	radius(radius),
	Object(position, material)
{
	scale = radius;
}

bool Sphere::Intersect(const Ray& ray, float min, float max, RayPayload& payload) {
	float a = ray.GetDirection().Dot(ray.GetDirection());
	float b = 2.0 * ray.GetDirection().Dot(ray.GetOrigin() - position);
	float c = (ray.GetOrigin() - position).Dot((ray.GetOrigin() - position)) - scale * scale;
	float d = b * b - 4.0 * a * c;

	if (d >= 0) {
		float t = (-b - sqrt(d)) / (2.0 * a);
		if (t <= max && t >= min) {
			payload.t = t;
			payload.point = ray.GetPointAt(payload.t);
			payload.normal = position - payload.point;
			payload.normal.Normalize();	
			payload.frontFace = payload.normal.Dot(ray.GetDirection()) < 0 ? false : true;
			payload.material = &material;
			payload.object = this;
			return true;
		}
	}

	return false;
}

}
