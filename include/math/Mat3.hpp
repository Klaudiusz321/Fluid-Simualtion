#pragma once

#include "Vec3.hpp"
#include <cmath>
#include <cstring>

/// Column-major 3x3 matrix (for 2D transforms with homogeneous coords, or 3D rotations)
struct Mat3 {
    float m[9]; // column-major: m[col*3 + row]

    Mat3() { std::memset(m, 0, sizeof(m)); }

    static Mat3 identity() {
        Mat3 result;
        result.m[0] = 1.0f; result.m[4] = 1.0f; result.m[8] = 1.0f;
        return result;
    }

    // Access element at (row, col)
    float& at(int row, int col) { return m[col * 3 + row]; }
    float  at(int row, int col) const { return m[col * 3 + row]; }

    // Matrix * Vector
    Vec3 operator*(const Vec3& v) const {
        return Vec3(
            m[0] * v.x + m[3] * v.y + m[6] * v.z,
            m[1] * v.x + m[4] * v.y + m[7] * v.z,
            m[2] * v.x + m[5] * v.y + m[8] * v.z
        );
    }

    // Matrix * Matrix
    Mat3 operator*(const Mat3& rhs) const {
        Mat3 result;
        for (int col = 0; col < 3; ++col) {
            for (int row = 0; row < 3; ++row) {
                result.m[col * 3 + row] =
                    m[0 * 3 + row] * rhs.m[col * 3 + 0] +
                    m[1 * 3 + row] * rhs.m[col * 3 + 1] +
                    m[2 * 3 + row] * rhs.m[col * 3 + 2];
            }
        }
        return result;
    }

    // 2D rotation matrix (around Z axis)
    static Mat3 rotation(float angleRad) {
        Mat3 r = identity();
        float c = std::cos(angleRad);
        float s = std::sin(angleRad);
        r.m[0] = c;  r.m[3] = -s;
        r.m[1] = s;  r.m[4] = c;
        return r;
    }

    // 2D scale
    static Mat3 scale(float sx, float sy) {
        Mat3 r = identity();
        r.m[0] = sx;
        r.m[4] = sy;
        return r;
    }

    // Transpose
    Mat3 transposed() const {
        Mat3 result;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                result.m[j * 3 + i] = m[i * 3 + j];
        return result;
    }

    // Determinant
    float determinant() const {
        return m[0] * (m[4] * m[8] - m[7] * m[5])
             - m[3] * (m[1] * m[8] - m[7] * m[2])
             + m[6] * (m[1] * m[5] - m[4] * m[2]);
    }
};
