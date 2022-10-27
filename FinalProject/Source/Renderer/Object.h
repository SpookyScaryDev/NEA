#pragma once

#include <string>
#include <Maths/Vector3f.h>
#include <Maths/Matrix4x4f.h>
#include <Maths/Ray.h>
#include <Renderer/RayPayload.h>
#include <Renderer/Material.h>

#include <nlohmann/json.hpp>

namespace Prototype {

class Object {
public:
	                    Object(Vector3f position, Material material);

	static Object*         LoadFromJSON(nlohmann::json data);
	virtual nlohmann::json ToJSON() = 0;

	virtual bool        Intersect(const Ray& ray, float min, float max, RayPayload& payload) = 0;

	Vector3f            GetPosition() const;
	void                SetPosition(const Vector3f& position);

	Vector3f            GetRotation() const;
	void                SetRotation(const Vector3f& rotation);

	Vector3f            GetScale() const;
	void                SetScale(const Vector3f& scale);

	Material            material;
	bool                show;
	int                 id;
	std::string         name;

protected:
	Vector3f            mPosition;
	Vector3f            mRotation;
	Vector3f            mScale;

	Matrix4x4f          mTransform;
	bool                mDirty;
};

}