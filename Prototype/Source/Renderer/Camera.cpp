#include "Camera.h"

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>

namespace Prototype {

Camera::Camera(float aspectRatio, float focalLength, Vector3f pos) {
	Create(aspectRatio, focalLength, pos);
}

void Camera::Create(float aspectRatio, float focalLength, Vector3f pos) {
	mAspectRatio = aspectRatio;
	mFocalLength = focalLength;
	position = pos;

	mViewportVertical = { 0, -1, 0 };
	mViewportHorizontal = { mAspectRatio, 0, 0 };
	mViewportTopLeft = mViewportHorizontal / -2 - mViewportVertical / 2 + Vector3f(0, 0, mFocalLength);
}
	
Vector3f Camera::GetViewportPos(Vector2f screenPos) const {
	Vector3f viewportPos = mViewportTopLeft + mViewportHorizontal * screenPos.x + mViewportVertical * screenPos.y - position;
	return viewportPos;
}

Vector2f Camera::GetScreenPos(Vector3f viewportPos) const {
	Vector2f screenPos;
	screenPos.x = (viewportPos.x - mViewportTopLeft.x + position.x) / mViewportHorizontal.x;
	screenPos.y = (viewportPos.y - mViewportTopLeft.y + position.y) / mViewportVertical.y;
	return screenPos;
}

}