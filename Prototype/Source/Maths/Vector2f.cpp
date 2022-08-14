#include "Vector2f.h"

#include <math.h>
#include <assert.h>

namespace Prototype {

Vector2f::Vector2f(float fx, float fy) :
    x(fx), y(fy) {}

void Vector2f::Set(float fx, float fy) {
    x = fx;
    y = fy;
}

float Vector2f::Magnitude() const {
    return sqrt((x * x) + (y * y));
}

void Vector2f::Normalize() {
    float magnitude = Magnitude();
    x /= magnitude;
    y /= magnitude;
}

float Vector2f::Dot(const Vector2f& vector) const {
    return (x * vector.x) + (y * vector.y);
}

Vector2f Vector2f::operator+(const Vector2f& vector) const {
    return Vector2f(x + vector.x, y + vector.y);
}

Vector2f& Vector2f::operator+=(const Vector2f& vector) {
    x += vector.x;
    y += vector.y;

    return *this;
}

Vector2f Vector2f::operator-(const Vector2f& vector) const {
    return Vector2f(x - vector.x, y - vector.y);
}

Vector2f& Vector2f::operator-=(const Vector2f& vector) {
    x -= vector.x;
    y -= vector.y;

    return *this;
}

Vector2f Vector2f::operator*(const float scalar) const {
    return Vector2f(x * scalar, y * scalar);
}

Vector2f& Vector2f::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;

    return *this;
}

Vector2f Vector2f::operator/(const float scalar) const {
    assert(scalar != 0);
    return Vector2f(x / scalar, y / scalar);
}

Vector2f& Vector2f::operator/=(float scalar) {
    assert(scalar != 0);
    x /= scalar;
    y /= scalar;

    return *this;
}

Vector2f operator*(const float scalar, const Vector2f& vector) {
    return vector * scalar;
}

Vector2f operator/(const float scalar, const Vector2f& vector) {
    assert(scalar != 0);
    return vector / scalar;
}

}
