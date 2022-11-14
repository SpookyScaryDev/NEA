#pragma once

#include <string>
#include <vector>
#include <Renderer/Object.h>
#include <Renderer/Triangle.h>

namespace Prototype {

class Mesh : Object {
public:
	                         Mesh(Vector3f position, const char* filePath, Material material);
	virtual nlohmann::json   ToJSON() override;
	void                     LoadFromFile(const char* filePath);

	virtual bool             Intersect(const Ray& ray, float min, float max, RayPayload& payload) override;
	virtual Vector3f         Sample(const Vector3f& point, float& pdf, std::mt19937& rnd) override;

private:
	std::vector<std::string> Split(const std::string& string, char character); // TODO: rethink this

	std::string              mFilePath;

	std::vector<Triangle*>   mFaces;
	Vector3f                 mMin;    // For AABB. TODO: don't leave this here!
	Vector3f                 mMax;

	Vector3f                 mTransformedMin;
	Vector3f                 mTransformedMax;
	Vector3f                 mCenter;
	float                    mRadius;
};

}