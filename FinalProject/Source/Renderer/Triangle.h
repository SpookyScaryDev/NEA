#pragma once

#include <vector>
#include <Renderer/Object.h>
#include <Maths/Vector3f.h>

namespace Prototype {

	class Triangle : Object {
	public:
		                    Triangle(Vector3f position, Vector3f verticies[3], Material material);
							virtual nlohmann::json ToJSON() override { return nlohmann::json(); }; //TODO

		virtual bool        Intersect(const Ray& ray, float min, float max, RayPayload& payload) override;
		virtual Vector3f    Sample(const Vector3f& point, float& pdf, std::mt19937& rnd) override { return Vector3f(); }; //TODO

	private:
		Vector3f            mVerticies[3];
		Vector3f            mTransformedVerticies[3];

	};

}