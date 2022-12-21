#pragma once

#include "nlohmann/json.hpp"

namespace Prototype {

class Vector3f {
public:
                        Vector3f(float fx = 0.0, float fy = 0.0, float fz = 0.0);

    std::string         ToString() const;

    static Vector3f     LoadFromJSON(nlohmann::json data);
    nlohmann::json      ToJSON();

    void                Set(float fx, float fy, float fz);

    float               Magnitude()                         const;
    void                Normalize();
    float               Dot(const Vector3f& vector)         const;
    Vector3f            Cross(const Vector3f& vector)       const;

    float&              operator[](int i);

    Vector3f            operator+(const Vector3f& vector)   const;
    Vector3f&           operator+=(const Vector3f& vector);

    Vector3f            operator-(const Vector3f& vector)   const;
    Vector3f&           operator-=(const Vector3f& vector);

    Vector3f            operator*(const float scalar)       const;
    Vector3f            operator*(const Vector3f& vector)   const;
    Vector3f&           operator*=(float scalar);

    Vector3f            operator/(const float scalar)       const;
    Vector3f&           operator/=(float scalar);

    bool                operator==(const Vector3f& vector)  const;
    bool                operator!=(const Vector3f& vector)  const;

    float           x;
    float           y;
    float           z;
};

Vector3f     operator*(const float scalar, const Vector3f& vector);

typedef Vector3f Colour;

}
