#include "Sphere.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <Maths/Sampling.h>
#include <Renderer/Material.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Prototype {

Sphere::Sphere(Vector3f position, float radius, Material material) :
	radius(radius),
	Object(position, material, ObjectType::Sphere)
{
	mScale = radius;
}

nlohmann::json Sphere::ToJSON() {
	json data = Object::ToJSON();
	data["type"] = "sphere";
	data["radius"] = mScale.x;

	return data;
}

bool Sphere::Intersect(const Ray& ray, float min, float max, RayPayload& payload) {
	float a = ray.GetDirection().Dot(ray.GetDirection());
	float b = 2.0 * ray.GetDirection().Dot(ray.GetOrigin() - mPosition);
	float c = (ray.GetOrigin() - mPosition).Dot((ray.GetOrigin() - mPosition)) - mScale.x * mScale.x;
	float d = b * b - 4.0 * a * c;

	// Sphere was hit
	if (d >= 0) {
		// Work out the two intersection points
		float t1 = (-b - sqrt(d)) / (2.0 * a);
		float t2 = (-b + sqrt(d)) / (2.0 * a);

		// Let t be the closer point which lies within the ray range
		float t = 0;
		float t2Final = 0;
		if (t1 <= max && t1 >= min) {
			t = t1;
			t2Final = t2;
		}
		else if (t2 <= max && t2 >= min) t = t2;
		else return false;

		// If t is within the acceptable range
		if (t <= max && t >= min) {
			payload.t = t;
			payload.t2 = t2Final;
			payload.point = ray.GetPointAt(payload.t);
			payload.normal = payload.point - mPosition;
			payload.normal.Normalize();	
			if (payload.normal.Dot(ray.GetDirection()) > 0) {
				// Came from inside
				payload.frontFace = false;
			}
			else {
				// Came from outside
				payload.frontFace = true;
			}
			payload.material = &material;
			payload.object = this;
			return true;
		}
	}

	return false;
}

Vector3f Sphere::Sample(const Vector3f& point, float& pdf, std::mt19937& rnd) {
	return Sampling::SamplePointInCone(point, mPosition, mScale.x, pdf, rnd);
}

}
