#pragma once

namespace Prototype {

class Vector3f {
public:
                        Vector3f(float fx = 0.0, float fy = 0.0, float fz = 0.0);

    void                Set(float fx, float fy, float fz);

    float               Magnitude()                        const;
    void                Normalize();
    float               Dot(const Vector3f& vector)        const;
    Vector3f&           Cross(const Vector3f& vector)      const;

    Vector3f            operator+(const Vector3f& vector)  const;
    Vector3f&           operator+=(const Vector3f& vector);

    Vector3f            operator-(const Vector3f& vector)  const;
    Vector3f&           operator-=(const Vector3f& vector);

    Vector3f            operator*(const float scalar)      const;
    Vector3f            operator*(const Vector3f& vector)  const;
    Vector3f&           operator*=(float scalar);

    Vector3f            operator/(const float scalar)      const;
    Vector3f&           operator/=(float scalar);

    float              x;
    float              y;
    float              z;
};

Vector3f     operator*(const float scalar, const Vector3f& vector);
Vector3f     operator/(const float scalar, const Vector3f& vector);

typedef Vector3f Colour;
typedef Vector3f Point3f;
    
}
