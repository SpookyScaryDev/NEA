#pragma once

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>

namespace Prototype {

class Camera {
public:
	                    Camera(float aspectRatio = 1, float focalLength = 1, Vector3f pos = {0, 0, 0});
	void                Create(float aspectRatio, float focalLength, Vector3f pos);
	
	Vector3f            GetViewportPos(Vector2f screenPos) const;  // Translates from pixel coordinates to their location in 3d space.
	Vector2f            GetScreenPos(Vector3f viewportPos) const;  // Translates a position from the viewport in 3d space into screen space.

	Vector3f            position;

private:
	float               mAspectRatio;
	float               mFocalLength;

	Vector3f            mViewportBottomLeft;
	Vector3f            mViewportHorizontal;
	Vector3f            mViewportVertical;
};

}