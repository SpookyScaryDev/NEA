#include "Vector2f.h"

#include <math.h>
#include <sstream>
#include <Error.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Prototype {

Vector2f::Vector2f(float fx, float fy) :
    x(fx), y(fy) {}

std::string Vector2f::ToString() const {
    std::stringstream stream;
    stream << "(";
    stream << std::setprecision(3) << x << ", ";
    stream << std::setprecision(3) << y;
    stream << ")";

    return stream.str();
}

Vector2f Vector2f::LoadFromJSON(json data) {
    Vector2f vector;
    int element = 0;

    for (json::iterator i = data.begin(); i != data.end(); i++) {
        vector[element] = *i;
        element++;
    }

    return vector;
}

nlohmann::json Vector2f::ToJSON() {
    json data = { x, y };
    return data;
}

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

float& Vector2f::operator[](int i) {
    if (i == 0) return x;
    if (i == 1) return y;
    else {
        ERROR("Element out of range");
    }
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

Vector2f Vector2f::operator*(const Vector2f& vector) const {
    return Vector2f(x * vector.x, y * vector.y);
}

Vector2f Vector2f::operator/(const float scalar) const {
    ASSERT(scalar != 0, "Tried to divide a vector by 0!");
    return Vector2f(x / scalar, y / scalar);
}

Vector2f& Vector2f::operator/=(float scalar) {
    ASSERT(scalar != 0, "Tried to divide a vector by 0!");
    x /= scalar;
    y /= scalar;

    return *this;
}

bool Vector2f::operator==(const Vector2f& vector) const {
    return x == vector.x && y == vector.y;
}

bool Vector2f::operator!=(const Vector2f& vector) const {
    return !(*this == vector);
}

Vector2f operator*(const float scalar, const Vector2f& vector) {
    return vector * scalar;
}

}
