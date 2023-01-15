#pragma once

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>

namespace rtos {

struct Line2D {
	Vector2f start;
	Vector2f end;

	Colour colour;
	float startAlpha;
	float endAlpha;
};

struct Line3D {
	Vector3f start;
	Vector3f end;

	Colour colour;
	float startAlpha;
	float endAlpha;
};

}