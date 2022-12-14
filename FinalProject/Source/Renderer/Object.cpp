#include "Object.h"

#include <Renderer/Sphere.h>
#include <Renderer/Mesh.h>
#include <Renderer/DivergingLens.h>
#include <Renderer/ConvergingLens.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Prototype {
Object::Object(Vector3f position, Material material, ObjectType type) :
	mPosition(position), material(material), mScale(Vector3f(1, 1, 1)) 
{
	show = true;
	lockAspectRatio = true;
	mDirty = true;
	mTransform = Matrix4x4f::Identity();
	mType = type;
}

Object* Object::LoadFromJSON(nlohmann::json data) {
    std::string type = data["type"];
    Object* object;

    Vector3f position = Vector3f::LoadFromJSON(data["position"]);
    Material material = Material::LoadFromJSON(data["material"]);

    if (type == "sphere") {
        float radius = data["radius"];
        object = (Object*) new Sphere(position, radius, material);
    }
    if (type == "mesh") {
        std::string filePath = data["path"].get<std::string>();
        object = (Object*) new Mesh(position, filePath.c_str(), material);
		object->SetScale(Vector3f::LoadFromJSON(data["scale"]));
    }
	if (type == "divergingLens") {
		object = (Object*) new DivergingLens(position, data["width"], data["curvature"], material);
	}
	if (type == "convergingLens") {
		object = (Object*) new ConvergingLens(position, data["width"], material);
	}

	object->name = data["name"];
	object->show = data["show"];
	if (data.contains("lockAspectRatio")) object->lockAspectRatio = data["lockAspectRatio"];
	object->SetRotation(Vector3f::LoadFromJSON(data["rotation"]));
	object->SetScale(Vector3f::LoadFromJSON(data["scale"]));

    return object;
}

nlohmann::json Object::ToJSON() {
	json data = json();
	data["name"] = name;
	data["position"] = mPosition.ToJSON();
	data["rotation"] = mRotation.ToJSON();
	data["scale"] = mScale.ToJSON();
	data["material"] = material.ToJSON();
	data["show"] = show;
	data["lockAspectRatio"] = lockAspectRatio;

	return data;
}

Vector3f Object::GetPosition() const {
	return mPosition;
}

void Object::SetPosition(const Vector3f& position) {
	mPosition = position;
	mDirty = true;
}

Vector3f Object::GetRotation() const {
	return mRotation;
}

void Object::SetRotation(const Vector3f& rotation) {
	mRotation = rotation;
	mDirty = true;
}

Vector3f Object::GetScale() const {
	return mScale;
}

void Object::SetScale(const Vector3f& scale) {
	mScale = scale;
	mDirty = true;
}

ObjectType Object::GetType() const {
	return mType;
}

}


