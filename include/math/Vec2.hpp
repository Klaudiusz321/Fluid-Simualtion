#pragma once

#include <cmath>
#include <iostream>

struct Vec2 {
    float x, y;

    constexpr Vec2() : x(0.0f), y(0.0f) {}
    constexpr Vec2(float _x, float _y) : x(_x), y(_y) {}
    constexpr explicit Vec2(float v) : x(v), y(v) {}

    // Arithmetic operators
    inline Vec2 operator+(const Vec2& rhs) const { return Vec2(x + rhs.x, y + rhs.y); }
    inline Vec2 operator-(const Vec2& rhs) const { return Vec2(x - rhs.x, y - rhs.y); }
    inline Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    inline Vec2 operator/(float s) const { float inv = 1.0f / s; return Vec2(x * inv, y * inv); }
    inline Vec2 operator-() const { return Vec2(-x, -y); }

    // Compound assignment
    inline Vec2& operator+=(const Vec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
    inline Vec2& operator-=(const Vec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
    inline Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    inline Vec2& operator/=(float s) { float inv = 1.0f / s; x *= inv; y *= inv; return *this; }

    // Dot product
    inline float dot(const Vec2& rhs) const { return x * rhs.x + y * rhs.y; }

    // Cross product (2D: returns scalar)
    inline float cross(const Vec2& rhs) const { return x * rhs.y - y * rhs.x; }

    // Length
    inline float lengthSq() const { return x * x + y * y; }
    inline float length() const { return std::sqrt(lengthSq()); }

    // Normalized vector
    inline Vec2 normalized() const {
        float len = length();
        return (len > 1e-6f) ? (*this / len) : Vec2(0.0f, 0.0f);
    }

    // Reflect across a normal
    inline Vec2 reflect(const Vec2& normal) const {
        return *this - normal * (2.0f * this->dot(normal));
    }

    // Perpendicular vector (rotate 90 degrees CCW)
    inline Vec2 perp() const { return Vec2(-y, x); }

    // Component-wise min/max
    static inline Vec2 min(const Vec2& a, const Vec2& b) {
        return Vec2(std::fmin(a.x, b.x), std::fmin(a.y, b.y));
    }
    static inline Vec2 max(const Vec2& a, const Vec2& b) {
        return Vec2(std::fmax(a.x, b.x), std::fmax(a.y, b.y));
    }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        os << "(" << v.x << ", " << v.y << ")";
        return os;
    }
};

// Left-multiply by scalar
inline Vec2 operator*(float s, const Vec2& v) { return Vec2(v.x * s, v.y * s); }
