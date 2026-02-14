#pragma once

#include <cmath>
#include <iostream>

struct Vec3 {
    float x, y, z;

    constexpr Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    constexpr Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    constexpr explicit Vec3(float v) : x(v), y(v), z(v) {}

    // Arithmetic operators
    inline Vec3 operator+(const Vec3& rhs) const { return Vec3(x + rhs.x, y + rhs.y, z + rhs.z); }
    inline Vec3 operator-(const Vec3& rhs) const { return Vec3(x - rhs.x, y - rhs.y, z - rhs.z); }
    inline Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    inline Vec3 operator/(float s) const { float inv = 1.0f / s; return Vec3(x * inv, y * inv, z * inv); }
    inline Vec3 operator-() const { return Vec3(-x, -y, -z); }

    // Compound assignment
    inline Vec3& operator+=(const Vec3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
    inline Vec3& operator-=(const Vec3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
    inline Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    inline Vec3& operator/=(float s) { float inv = 1.0f / s; x *= inv; y *= inv; z *= inv; return *this; }

    // Dot product
    inline float dot(const Vec3& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }

    // Cross product
    inline Vec3 cross(const Vec3& rhs) const {
        return Vec3(
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        );
    }

    // Length
    inline float lengthSq() const { return x * x + y * y + z * z; }
    inline float length() const { return std::sqrt(lengthSq()); }

    // Normalized vector
    inline Vec3 normalized() const {
        float len = length();
        return (len > 1e-6f) ? (*this / len) : Vec3(0.0f, 0.0f, 0.0f);
    }

    // Reflect across a normal
    inline Vec3 reflect(const Vec3& normal) const {
        return *this - normal * (2.0f * this->dot(normal));
    }

    // Component-wise min/max
    static inline Vec3 min(const Vec3& a, const Vec3& b) {
        return Vec3(std::fmin(a.x, b.x), std::fmin(a.y, b.y), std::fmin(a.z, b.z));
    }
    static inline Vec3 max(const Vec3& a, const Vec3& b) {
        return Vec3(std::fmax(a.x, b.x), std::fmax(a.y, b.y), std::fmax(a.z, b.z));
    }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Vec3& v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};

// Left-multiply by scalar
inline Vec3 operator*(float s, const Vec3& v) { return Vec3(v.x * s, v.y * s, v.z * s); }
