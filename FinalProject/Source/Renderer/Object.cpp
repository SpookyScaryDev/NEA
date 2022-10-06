#include "Object.h"

namespace Prototype {
Object::Object(Vector3f position, Material material) :
	mPosition(position), material(material), mScale(Vector3f(1, 1, 1)) 
{
	show = true;
	mDirty = true;
	mTransform = Matrix4x4f::Identity();
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

}


