#pragma once

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>

#include <nlohmann/json.hpp>

namespace Prototype {

class Camera {
public:
	                    Camera(float aspectRatio = 1, float focalLength = 1, Vector3f pos = {0, 0, 0});
	void                Create(float aspectRatio, float focalLength, Vector3f pos);

	static Camera       LoadFromJSON(nlohmann::json data);
	nlohmann::json      ToJSON();
	
	Vector3f            GetViewportPos(Vector2f screenPos) const;  // Translates from pixel coordinates to their location in 3d space.
	Vector2f            GetScreenPos(Vector3f viewportPos) const;

	Vector3f            position;

private:
	float               mAspectRatio;
	float               mFocalLength;

	Vector3f            mViewportBottomLeft;
	Vector3f            mViewportHorizontal;
	Vector3f            mViewportVertical;
};

}