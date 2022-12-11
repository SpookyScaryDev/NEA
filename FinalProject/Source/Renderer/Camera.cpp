#include "Camera.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>
#include <Maths/Matrix4x4f.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Prototype {

Camera::Camera(float aspectRatio, float focalLength, Vector3f pos) {
	Create(aspectRatio, focalLength, pos);
}

void Camera::Create(float aspectRatio, float focalLength, Vector3f pos) {
	mAspectRatio = aspectRatio;
	mFocalLength = focalLength;
	mPosition = pos;

	mForward = Vector3f(0, 0, -1);
	mViewportVertical = { 0, 2, 0 };
	mViewportHorizontal = { 2 * mAspectRatio, 0, 0 };
	mViewportBottomLeft = mViewportHorizontal / -2 - mViewportVertical / 2 + Vector3f(0, 0, -mFocalLength);

	mDirty = true;
}

Camera Camera::LoadFromJSON(json data) {
	Camera camera(data["aspectRatio"], data["focalLength"], Vector3f::LoadFromJSON(data["position"]));
	return camera;
}

json Camera::ToJSON() {
	json data;
	data["aspectRatio"] = mAspectRatio;
	data["focalLength"] = mFocalLength;
	data["position"] = mPosition.ToJSON();

	return data;
}
	
Vector3f Camera::GetViewportPos(Vector2f screenPos) const {
	Vector3f viewportPos = (mViewportBottomLeft) + mViewportHorizontal * screenPos.x + mViewportVertical * (1-screenPos.y);
	return viewportPos;
}

Vector2f Camera::GetScreenPos(Vector3f point) const {
	Vector3f toPoint = point - mPosition;
	Vector3f viewportPlaneNormal  = mViewportBottomLeft + 0.5 * mViewportVertical + 0.5 * mViewportHorizontal;

	float d = viewportPlaneNormal.Dot(mViewportBottomLeft + mPosition);

	float lambda = (d - viewportPlaneNormal.Dot(mPosition)) / (viewportPlaneNormal.Dot(toPoint));

	Vector3f posOnViewport = mPosition + lambda * toPoint;

	Vector2f screenPos = Vector2f((posOnViewport.x - (mViewportBottomLeft.x + mPosition.x)) / mViewportHorizontal.x,
		                          1-((posOnViewport.y - (mViewportBottomLeft.y + mPosition.y)) / mViewportVertical.y));

	//Vector3f posOnViewport2 = GetViewportPos(screenPos);

	//printf("(%f, %f, %f) (%f, %f, %f)\n", posOnViewport.x, posOnViewport.y, posOnViewport.z, posOnViewport2.x, posOnViewport2.y, posOnViewport2.z);

	return screenPos;
}

void Camera::Update() {
	//if (mDirty) {
	//	mDirty = false;
	//}
}

void Camera::MoveInDirection(Vector3f transform) {
	Vector3f right = mViewportHorizontal;
	Vector3f up = mViewportVertical;
	right.Normalize();
	up.Normalize();

	mPosition += transform.x * right + transform.y * up + transform.z * mForward;
	mDirty = true;
}

void Camera::LookAt(Vector3f target) {
	mForward = target - GetPosition();
	mForward.Normalize();

	mViewportHorizontal = mForward.Cross(Vector3f(0, 1, 0));
	mViewportHorizontal.Normalize();
	mViewportHorizontal = mViewportHorizontal * 2 * mAspectRatio;

	mViewportVertical = mViewportHorizontal.Cross(mForward);
	mViewportVertical.Normalize();
	mViewportVertical = mViewportVertical * 2;

	mViewportBottomLeft = mFocalLength * mForward - 0.5 * mViewportHorizontal - 0.5 * mViewportVertical;
}

Vector3f Camera::GetPosition() const {
	return mPosition;
}

void Camera::SetPosition(const Vector3f& position) {
	mPosition = position;
	mDirty = true;
}

}