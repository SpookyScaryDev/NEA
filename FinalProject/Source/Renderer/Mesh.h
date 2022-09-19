#pragma once

#include <vector>
#include <Renderer/Object.h>
#include <Renderer/Triangle.h>

namespace Prototype {

class Mesh : Object {
public:
	                         Mesh(Vector3f position, const char* filePath, Material material);
	void                     LoadFromFile(const char* filePath);
	virtual bool             Intersect(const Ray& ray, float min, float max, RayPayload& payload) override;

private:
	std::vector<std::string> Split(const std::string& string, char character); // TODO: rethink this

	std::vector<Triangle*>   mFaces;
	Vector3f                 mMin;    // For AABB. TODO: don't leave this here!
	Vector3f                 mMax;
};

}