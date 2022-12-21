#pragma once

#include <string>
#include "nlohmann/json.hpp"

namespace Prototype {

class Vector2f {
public:
                        Vector2f(float fx = 0.0, float fy = 0.0);

    std::string         ToString() const;

    static Vector2f     LoadFromJSON(nlohmann::json data);
    nlohmann::json      ToJSON();

    void                Set(float fx, float fy);

    float               Magnitude() const;
    void                Normalize();
    float               Dot(const Vector2f& vector)       const;

    float&              operator[](int i);

    Vector2f            operator+(const Vector2f& vector) const;
    Vector2f&           operator+=(const Vector2f& vector);

    Vector2f            operator-(const Vector2f& vector) const;
    Vector2f&           operator-=(const Vector2f& vector);

    Vector2f            operator*(const float scalar)     const;
    Vector2f&           operator*=(float scalar);

    Vector2f            operator*(const Vector2f& vector) const;

    Vector2f            operator/(const float scalar)     const;
    Vector2f&           operator/=(float scalar);

    bool                operator==(const Vector2f& vector)  const;
    bool                operator!=(const Vector2f& vector)  const;

    float               x;
    float               y;
};

Vector2f     operator*(const float scalar, const Vector2f& vector);

}
