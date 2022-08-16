#pragma once

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>

namespace Prototype {

class Camera {
public:
	                    Camera(float aspectRatio, float focalLength, Vector3f pos);
	void                Create(float aspectRatio, float focalLength, Vector3f pos);
	
	Vector3f            GetViewportPos(Vector2f screenPos) const;
	Vector2f            GetScreenPos(Vector3f viewportPos) const;

	Vector3f            position;

private:
	float               mAspectRatio;
	float               mFocalLength;

	Vector3f            mViewportTopLeft;
	Vector3f            mViewportHorizontal;
	Vector3f            mViewportVertical;
};

}