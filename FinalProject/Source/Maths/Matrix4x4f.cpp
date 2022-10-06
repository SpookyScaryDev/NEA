#include "Matrix4x4f.h"

#define _USE_MATH_DEFINES
#include <Error.h>
#include <cmath>

namespace Prototype {
    Matrix4x4f::Matrix4x4f() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                mData[i][j] = 0;
            }
        }
    }

    Matrix4x4f::Matrix4x4f(float data[4][4]) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                mData[i][j] = data[i][j];
            }
        }
    }

    Matrix4x4f Matrix4x4f::Zero() {
        return Matrix4x4f();
    }

    Matrix4x4f Matrix4x4f::Identity() {
        Matrix4x4f mat = Matrix4x4f();
        mat.SetValue(0, 0, 1);
        mat.SetValue(1, 1, 1);
        mat.SetValue(2, 2, 1);
        mat.SetValue(3, 3, 1);
        return mat;
    }

    Matrix4x4f Matrix4x4f::Translate(const Vector3f& vector) {
        Matrix4x4f mat = Identity();
        mat.SetValue(0, 3, vector.x);
        mat.SetValue(1, 3, vector.y);
        mat.SetValue(2, 3, vector.z);
        return mat;
    }

    Matrix4x4f Matrix4x4f::Rotate(const Vector3f& angles) {
        Vector3f radians = Vector3f(2*M_PI, 2 * M_PI, 2 * M_PI) - angles * (M_PI / 180);

        Matrix4x4f rx = Identity();
        rx.SetValue(1, 1, cos(radians.x));
        rx.SetValue(1, 2, -sin(radians.x));
        rx.SetValue(2, 1, sin(radians.x));
        rx.SetValue(2, 2, cos(radians.x));

        Matrix4x4f ry = Identity();
        ry.SetValue(0, 0, cos(radians.y));
        ry.SetValue(0, 2, sin(radians.y));
        ry.SetValue(2, 0, -sin(radians.y));
        ry.SetValue(2, 2, cos(radians.y));

        Matrix4x4f rz = Identity();
        rz.SetValue(0, 0, cos(radians.z));
        rz.SetValue(0, 1, -sin(radians.z));
        rz.SetValue(1, 0, sin(radians.z));
        rz.SetValue(1, 1, cos(radians.z));

        Matrix4x4f result = rx * ry * rz;
        return result;
    }

    Matrix4x4f Matrix4x4f::Scale(const Vector3f& scale) {
        Matrix4x4f mat = Identity();
        mat.SetValue(0, 0, scale.x);
        mat.SetValue(1, 1, scale.y);
        mat.SetValue(2, 2, scale.z);
        return mat;
    }


    float Matrix4x4f::GetValue(int row, int column) const {
        ASSERT(row >= 0 || row < 4, "Operator out of range");
        ASSERT(column >= 0 || column < 4, "Operator out of range");
        return mData[row][column];
    }

    void Matrix4x4f::SetValue(int row, int column, float value) {
        ASSERT(row >= 0 || row < 4, "Operator out of range");
        ASSERT(column >= 0 || column < 4, "Operator out of range");
        mData[row][column] = value;
    }

    float(&Matrix4x4f::operator[](int i))[4]{
        ASSERT(i >= 0 || i < 4, "Operator out of range");
        return mData[i];
    }

    Matrix4x4f Matrix4x4f::operator+(const Matrix4x4f& matrix) const {
        Matrix4x4f result = Matrix4x4f();
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.SetValue(i, j, matrix.GetValue(i, j) + GetValue(i, j));
            }
        }
        return result;
    }

    Matrix4x4f& Matrix4x4f::operator+=(const Matrix4x4f& matrix) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                SetValue(i, j, matrix.GetValue(i, j) + GetValue(i, j));
            }
        }
        return *this;
    }

    Matrix4x4f Matrix4x4f::operator-(const Matrix4x4f& matrix) const {
        Matrix4x4f result = Matrix4x4f();
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.SetValue(i, j, GetValue(i, j) - matrix.GetValue(i, j));
            }
        }
        return result;
    }

    Matrix4x4f& Matrix4x4f::operator-=(const Matrix4x4f& matrix) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                SetValue(i, j, GetValue(i, j) - matrix.GetValue(i, j));
            }
        }
        return *this;
    }

    Matrix4x4f Matrix4x4f::operator*(const float scalar) const {
        Matrix4x4f result = Matrix4x4f();
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.SetValue(i, j, GetValue(i, j) * scalar);
            }
        }
        return result;
    }

    Matrix4x4f Matrix4x4f::operator*(const Matrix4x4f& matrix) const {
        Matrix4x4f result = Matrix4x4f();
        for (int row = 0; row < 4; row++)
        {
            for (int column = 0; column < 4; column++)
            {
                float element = 0;
                for (int i = 0; i < 4; i++)
                {
                    element += GetValue(row, i) * matrix.GetValue(i, column);
                }
                result.SetValue(row, column, element);
            }
        }
        return result;
    }

    Vector3f Matrix4x4f::operator*(const Vector3f& vector) const {
        Vector3f vec;
        vec.x = GetValue(0, 0) * vector.x + GetValue(0, 1) * vector.y + GetValue(0, 2) * vector.z + GetValue(0, 3);
        vec.y = GetValue(1, 0) * vector.x + GetValue(1, 1) * vector.y + GetValue(1, 2) * vector.z + GetValue(1, 3);
        vec.z = GetValue(2, 0) * vector.x + GetValue(2, 1) * vector.y + GetValue(2, 2) * vector.z + GetValue(2, 3);
        return vec;
    }

    Matrix4x4f& Matrix4x4f::operator*=(float scalar) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                SetValue(i, j, GetValue(i, j) * scalar);
            }
        }
        return *this;
    }

    Matrix4x4f Matrix4x4f::operator/(const float scalar) const {
        Matrix4x4f result = Matrix4x4f();
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.SetValue(i, j, GetValue(i, j) / scalar);
            }
        }
        return result;
    }

    Matrix4x4f& Matrix4x4f::operator/=(float scalar) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                SetValue(i, j, GetValue(i, j) / scalar);
            }
        }
        return *this;
    }

    bool Matrix4x4f::operator==(const Matrix4x4f& matrix) const {
        bool equal = true;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (GetValue(i, j) != matrix.GetValue(i, j)) equal = false;
            }
        }
        return equal;
    }

    bool Matrix4x4f::operator!=(const Matrix4x4f& matrix) const {
        return !(*this == matrix);
    }

    Matrix4x4f operator*(const float scalar, const Matrix4x4f& matrix) {
        return matrix * scalar;
    }

    Matrix4x4f operator/(const float scalar, const Matrix4x4f& matrix) {
        return matrix / scalar;
    }
}
