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

	mViewportVertical = { 0, 2, 0 };
	mViewportHorizontal = { 2 * mAspectRatio, 0, 0 };
	mViewportBottomLeft = mViewportHorizontal / -2 - mViewportVertical / 2 + Vector3f(0, 0, -mFocalLength);
}
	
Vector3f Camera::GetViewportPos(Vector2f screenPos) const {
	Vector3f viewportPos = mViewportBottomLeft + mViewportHorizontal * screenPos.x + mViewportVertical * (1-screenPos.y) + position;
	return viewportPos;
}

}