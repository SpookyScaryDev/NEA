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
	mViewportVertical = { 0, 1, 0 };
	mViewportHorizontal = { mAspectRatio, 0, 0 };
	mViewportBottomLeft = mFocalLength * mForward - 0.5 * mViewportHorizontal - 0.5 * mViewportVertical;
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
	Vector3f viewportPlaneNormal = mForward;

	float d = viewportPlaneNormal.Dot(mViewportBottomLeft + mPosition);

	float lambda = (d - viewportPlaneNormal.Dot(mPosition)) / (viewportPlaneNormal.Dot(toPoint));

	Vector3f posOnViewport = lambda * toPoint;

	Vector2f screenPos1 = Vector2f((posOnViewport.x - (mViewportBottomLeft.x)) / mViewportHorizontal.x,
		                          1-((posOnViewport.y - (mViewportBottomLeft.y)) / mViewportVertical.y));

	float det = 1 / (mViewportHorizontal.x * mViewportVertical.y - mViewportHorizontal.y * mViewportVertical.x);
	Vector2f screenPos;
	Vector3f pos = posOnViewport - mViewportBottomLeft;
	screenPos.x = ( ( mViewportVertical.y * pos.x ) * det - (pos.y * mViewportVertical.x ) * det );
	screenPos.y = 1 - ( - (mViewportHorizontal.y * pos.x ) * det + (pos.y * mViewportHorizontal.x ) * det );

	Vector3f posOnViewport2 = GetViewportPos(screenPos);

	//printf("(%f, %f, %f) (%f, %f, %f)\n", posOnViewport.x, posOnViewport.y, posOnViewport.z, posOnViewport2.x, posOnViewport2.y, posOnViewport2.z);
	//printf("(%f, %f) (%f, %f)\n", screenPos1.x, screenPos1.y, screenPos.x, screenPos.y);

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
	mViewportHorizontal = mViewportHorizontal * mAspectRatio;

	mViewportVertical = mViewportHorizontal.Cross(mForward);
	mViewportVertical.Normalize();
	mViewportVertical = mViewportVertical;

	mViewportBottomLeft = mFocalLength * mForward - 0.5 * mViewportHorizontal - 0.5 * mViewportVertical;
}

Vector3f Camera::GetDirection() const {
	return mForward;
}

Vector3f Camera::GetPosition() const {
	return mPosition;
}

void Camera::SetPosition(const Vector3f& position) {
	mPosition = position;
	mDirty = true;
}

void Camera::SetAspectRatio(float aspectRatio) {
	mAspectRatio = aspectRatio;
	mViewportHorizontal.Normalize();
	mViewportHorizontal = mViewportHorizontal * mAspectRatio;
	mViewportBottomLeft = mFocalLength * mForward - 0.5 * mViewportHorizontal - 0.5 * mViewportVertical;
}

}