#pragma once

#include <Maths/Vector3f.h>

namespace rtos {

class Matrix4x4f {
public:
	                    Matrix4x4f();
	                    Matrix4x4f(float data[4][4]);

    std::string         ToString() const;

    static Matrix4x4f   Zero();
    static Matrix4x4f   Identity();

    static Matrix4x4f   Translate(const Vector3f& vector);
    static Matrix4x4f   Rotate(const Vector3f& angles);
    static Matrix4x4f   Scale(const Vector3f& scale);

    float               GetValue(int row, int column) const;
    void                SetValue(int row, int column, float value);

    float               (&operator[](int i))[4];

    Matrix4x4f          operator+(const Matrix4x4f& matrix) const;
    Matrix4x4f&         operator+=(const Matrix4x4f& matrix);

    Matrix4x4f          operator-(const Matrix4x4f& matrix) const;
    Matrix4x4f&         operator-=(const Matrix4x4f& matrix);

    Matrix4x4f          operator*(const float scalar)     const;
    Matrix4x4f          operator*(const Matrix4x4f& matrix) const;
    Vector3f            operator*(const Vector3f& vector) const;
    Matrix4x4f&         operator*=(float scalar);

    Matrix4x4f          operator/(const float scalar)     const;
    Matrix4x4f&         operator/=(float scalar);

    bool                operator==(const Matrix4x4f& matrix)  const;
    bool                operator!=(const Matrix4x4f& matrix)  const;

private:
	float               mData[4][4];
};

Matrix4x4f     operator*(const float scalar, const Matrix4x4f& matrix);

}
