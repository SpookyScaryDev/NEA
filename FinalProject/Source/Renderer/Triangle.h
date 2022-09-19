#pragma once

#include <vector>
#include <Renderer/Object.h>
#include <Maths/Vector3f.h>

namespace Prototype {

	class Triangle : Object {
	public:
		                    Triangle(Vector3f position, Vector3f verticies[3], Material material);
		virtual bool        Intersect(const Ray& ray, float min, float max, RayPayload& payload) override;

	private:
		Vector3f            mVerticies[3];

	};

}