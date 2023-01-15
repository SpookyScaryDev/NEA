#include "Ray.h"

#include <Maths/Vector3f.h>

namespace rtos {

Ray::Ray(const Vector3f& origin, const Vector3f& direction) :
    mOrigin(origin), 
    mDirection(direction)
{
    mDirection.Normalize();
}

Vector3f Ray::GetPointAt(float u) const {
    return mOrigin + mDirection * u;
}

Vector3f Ray::GetOrigin() const {
    return mOrigin;
}

Vector3f Ray::GetDirection() const {
    return mDirection;
}

}
