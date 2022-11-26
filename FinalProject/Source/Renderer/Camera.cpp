#include "Camera.h"

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Prototype {

Camera::Camera(float aspectRatio, float focalLength, Vector3f pos) {
	Create(aspectRatio, focalLength, pos);
}

void Camera::Create(float aspectRatio, float focalLength, Vector3f pos) {
	mAspectRatio = aspectRatio;
	mFocalLength = focalLength;
	position = pos;

	mViewportVertical = { 0, 2, 0 };
	mViewportHorizontal = { 2 * mAspectRatio, 0, 0 };
	mViewportBottomLeft = mViewportHorizontal / -2 - mViewportVertical / 2 + Vector3f(0, 0, -mFocalLength);
}

Camera Camera::LoadFromJSON(json data) {
	Camera camera(data["aspectRatio"], data["focalLength"], Vector3f::LoadFromJSON(data["position"]));
	return camera;
}

json Camera::ToJSON() {
	json data;
	data["aspectRatio"] = mAspectRatio;
	data["focalLength"] = mFocalLength;
	data["position"] = position.ToJSON();

	return data;
}
	
Vector3f Camera::GetViewportPos(Vector2f screenPos) const {
	Vector3f viewportPos = (mViewportBottomLeft) + mViewportHorizontal * screenPos.x + mViewportVertical * (1-screenPos.y);
	return viewportPos;
}

Vector2f Camera::GetScreenPos(Vector3f point) const {
	Vector3f toPoint = point - position;
	Vector3f viewportPlaneNormal  = mViewportBottomLeft + 0.5 * mViewportVertical + 0.5 * mViewportHorizontal;

	float d = viewportPlaneNormal.Dot(mViewportBottomLeft + position);

	float lambda = (d - viewportPlaneNormal.Dot(position)) / (viewportPlaneNormal.Dot(toPoint));

	Vector3f posOnViewport = position + lambda * toPoint;

	Vector2f screenPos = Vector2f((posOnViewport.x - (mViewportBottomLeft.x + position.x)) / mViewportHorizontal.x,
		                          1-((posOnViewport.y - (mViewportBottomLeft.y + position.y)) / mViewportVertical.y));

	//Vector3f posOnViewport2 = GetViewportPos(screenPos);

	//printf("(%f, %f, %f) (%f, %f, %f)\n", posOnViewport.x, posOnViewport.y, posOnViewport.z, posOnViewport2.x, posOnViewport2.y, posOnViewport2.z);

	return screenPos;
}

}