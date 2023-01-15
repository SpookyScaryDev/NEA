
#pragma once 

#include <Maths/Vector3f.h>

namespace rtos {

class Ray {
public:
                        Ray(const Vector3f& origin, const Vector3f& direction);
    Vector3f            GetPointAt(float t) const;
    Vector3f            GetOrigin() const;
    Vector3f            GetDirection() const;

private:
    Vector3f            mOrigin;
    Vector3f            mDirection;
};

}
