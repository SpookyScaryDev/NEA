#include "Sphere.h"

#include <math.h>
#include <Renderer/Material.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Prototype {

Sphere::Sphere(Vector3f position, float radius, Material material) :
	radius(radius),
	Object(position, material)
{
	mScale = radius;
}

nlohmann::json Sphere::ToJSON() {
	json data = json();
	data["name"] = name;
	data["type"] = "sphere";
	data["position"] = mPosition.ToJSON();
	data["rotation"] = mRotation.ToJSON();
	data["radius"] = mScale.x;
	data["material"] = material.ToJSON();

	return data;
}

bool Sphere::Intersect(const Ray& ray, float min, float max, RayPayload& payload) {
	float a = ray.GetDirection().Dot(ray.GetDirection());
	float b = 2.0 * ray.GetDirection().Dot(ray.GetOrigin() - mPosition);
	float c = (ray.GetOrigin() - mPosition).Dot((ray.GetOrigin() - mPosition)) - mScale.x * mScale.x;
	float d = b * b - 4.0 * a * c;

	if (d >= 0) {
		float t1 = (-b - sqrt(d)) / (2.0 * a);
		float t2 = (-b + sqrt(d)) / (2.0 * a);
		float t = 0;
		if (t1 <= max && t1 >= min) t = t1;
		else if (t2 <= max && t2 >= min) t = t2;
		else return false;

		if (t <= max && t >= min) {
			payload.t = t;
			payload.point = ray.GetPointAt(payload.t);
			payload.normal = payload.point - mPosition;
			payload.normal.Normalize();	
			if (payload.normal.Dot(ray.GetDirection()) > 0) {
				payload.frontFace = false;
			}
			else {
				payload.frontFace = true;
			}
			//payload.frontFace = payload.normal.Dot(ray.GetDirection()) < 0 ? false : true;
			payload.material = &material;
			payload.object = this;
			return true;
		}
	}

	return false;
}

}
