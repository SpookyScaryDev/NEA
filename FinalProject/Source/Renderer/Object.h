#pragma once

#include <string>
#include <Maths/Vector3f.h>
#include <Maths/Matrix4x4f.h>
#include <Maths/Ray.h>
#include <Renderer/RayPayload.h>
#include <Renderer/Material.h>

#include <nlohmann/json.hpp>

namespace Prototype {

enum class ObjectType {
	Sphere,
	Mesh,
	DivergingLens,
	Triangle,
	ConvergingLens
};

class Object {
public:
	                    Object(Vector3f position, Material material, ObjectType type);

	static Object*         LoadFromJSON(nlohmann::json data);
	virtual nlohmann::json ToJSON();

	virtual void        Update() = 0;

	virtual bool        Intersect(const Ray& ray, float min, float max, RayPayload& payload) = 0;

	virtual Vector3f    Sample(const Vector3f& point, float& pdf, std::mt19937& rnd) = 0;

	Vector3f            GetPosition() const;
	void                SetPosition(const Vector3f& position);

	Vector3f            GetRotation() const;
	void                SetRotation(const Vector3f& rotation);

	Vector3f            GetScale() const;
	void                SetScale(const Vector3f& scale);

	ObjectType          GetType() const;

	Material            material;
	bool                show;
	std::string         name;
	bool                lockAspectRatio;

protected:
	ObjectType          mType;

	Vector3f            mPosition;
	Vector3f            mRotation;
	Vector3f            mScale;

	Matrix4x4f          mTransform;
	bool                mDirty;
};

}