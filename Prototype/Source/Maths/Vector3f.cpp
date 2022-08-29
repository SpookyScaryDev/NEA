#include "Vector3f.h"

#include <math.h>
#include <assert.h>

namespace Prototype {

Vector3f::Vector3f(float fx, float fy, float fz) :
    x(fx), y(fy), z(fz) {}

void Vector3f::Set(float fx, float fy, float fz) {
    x = fx;
    y = fy;
    z = fz;
}

float Vector3f::Magnitude() const {
    return sqrt((x * x) + (y * y) + (z * z));
}

void Vector3f::Normalize() {
    float magnitude = Magnitude();
    x /= magnitude;
    y /= magnitude;
    z /= magnitude;
}

float Vector3f::Dot(const Vector3f& vector) const {
    return vector.x * x + vector.y * y + vector.z * z;
}

Vector3f& Vector3f::Cross(const Vector3f& vector) const {
    return Vector3f(y * vector.z - z * vector.y,
                    z * vector.x - x * vector.z,
                    x * vector.y - y * vector.x);
}

Vector3f Vector3f::operator+(const Vector3f& vector) const {
    return Vector3f(x + vector.x, y + vector.y, z + vector.z);
}

Vector3f& Vector3f::operator+=(const Vector3f& vector) {
    x += vector.x;
    y += vector.y;
    z += vector.z;

    return *this;
}

Vector3f Vector3f::operator-(const Vector3f& vector) const {
    return Vector3f(x - vector.x, y - vector.y, z - vector.z);
}

Vector3f& Vector3f::operator-=(const Vector3f& vector) {
    x -= vector.x;
    y -= vector.y;
    z -= vector.z;

    return *this;
}

Vector3f Vector3f::operator*(const float scalar) const {
    return Vector3f(x * scalar, y * scalar, z * scalar);
}

Vector3f Vector3f::operator*(const Vector3f& vector) const {
    return Vector3f(x * vector.x, y * vector.y, z * vector.z);
}

Vector3f& Vector3f::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;

    return *this;
}

Vector3f Vector3f::operator/(const float scalar) const {
    assert(scalar != 0);
    return Vector3f(x / scalar, y / scalar, z / scalar);
}

Vector3f& Vector3f::operator/=(float scalar) {
    assert(scalar != 0);
    x /= scalar;
    y /= scalar;
    z /= scalar;

    return *this;
}

bool Vector3f::operator==(const Vector3f& vector) const {
    return x == vector.x && y == vector.y && z == vector.z;
}

bool Vector3f::operator!=(const Vector3f& vector) const {
    return !(*this == vector);
}

Vector3f operator*(const float scalar, const Vector3f& vector) {
    return vector * scalar;
}

Vector3f operator/(const float scalar, const Vector3f& vector) {
    assert(scalar != 0);
    return vector / scalar;
}

}
