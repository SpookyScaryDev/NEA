#pragma once

#include <Renderer/Object.h>
#include <Maths/Ray.h>
#include <Renderer/Material.h>

namespace rtos {

class Sphere : Object {
public:
	                    Sphere(Vector3f position, float radius, Material material);
	virtual nlohmann::json ToJSON() override;

	virtual void        Update() override {};

	virtual bool        Intersect(const Ray& ray, float min, float max, RayPayload& payload) override;
	virtual Vector3f    Sample(const Vector3f& point, float& pdf, std::mt19937& rnd) override;

	float               radius;
};

}