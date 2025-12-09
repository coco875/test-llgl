#pragma once

#include <cmath>
#include <cstring>
#include <algorithm>
#include <limits>

namespace Math {

// 3D Vector
struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {
    }
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {
    }
    Vec3(float v) : x(v), y(v), z(v) {
    }

    float* data() {
        return &x;
    }
    const float* data() const {
        return &x;
    }

    float& operator[](int i) {
        return (&x)[i];
    }
    float operator[](int i) const {
        return (&x)[i];
    }

    Vec3 operator+(const Vec3& v) const {
        return { x + v.x, y + v.y, z + v.z };
    }
    Vec3 operator-(const Vec3& v) const {
        return { x - v.x, y - v.y, z - v.z };
    }
    Vec3 operator*(float s) const {
        return { x * s, y * s, z * s };
    }
    Vec3 operator/(float s) const {
        return { x / s, y / s, z / s };
    }
    Vec3 operator-() const {
        return { -x, -y, -z };
    }

    Vec3& operator+=(const Vec3& v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    Vec3& operator-=(const Vec3& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }
    Vec3& operator*=(float s) {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }
    float lengthSquared() const {
        return x * x + y * y + z * z;
    }

    Vec3 normalized() const {
        float len = length();
        return len > 0 ? *this / len : Vec3(0);
    }

    static float dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    }

    static Vec3 minVec(const Vec3& a, const Vec3& b) {
        return Vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
    }

    static Vec3 maxVec(const Vec3& a, const Vec3& b) {
        return Vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
    }
};

inline Vec3 operator*(float s, const Vec3& v) {
    return v * s;
}

// 2D Vector
struct Vec2 {
    float x, y;

    Vec2() : x(0), y(0) {
    }
    Vec2(float x, float y) : x(x), y(y) {
    }

    float* data() {
        return &x;
    }
    const float* data() const {
        return &x;
    }

    float& operator[](int i) {
        return (&x)[i];
    }
    float operator[](int i) const {
        return (&x)[i];
    }
};

// 4x4 Matrix (column-major order)
struct Mat4 {
    float m[16];

    Mat4() {
        identity();
    }

    float* data() {
        return m;
    }
    const float* data() const {
        return m;
    }

    void identity() {
        std::memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    float& operator()(int row, int col) {
        return m[col * 4 + row];
    }
    float operator()(int row, int col) const {
        return m[col * 4 + row];
    }

    Mat4 operator*(const Mat4& other) const {
        Mat4 result;
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                result.m[col * 4 + row] = 0.0f;
                for (int k = 0; k < 4; k++) {
                    result.m[col * 4 + row] += m[k * 4 + row] * other.m[col * 4 + k];
                }
            }
        }
        return result;
    }

    static Mat4 perspective(float fovY, float aspect, float nearPlane, float farPlane) {
        Mat4 result;
        float tanHalfFov = std::tan(fovY / 2.0f);

        result.m[0] = 1.0f / (aspect * tanHalfFov);
        result.m[5] = 1.0f / tanHalfFov;
        result.m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result.m[11] = -1.0f;
        result.m[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        result.m[15] = 0.0f;

        return result;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 r = Vec3::cross(f, up).normalized();
        Vec3 u = Vec3::cross(r, f);

        Mat4 result;
        result.m[0] = r.x;
        result.m[4] = r.y;
        result.m[8] = r.z;
        result.m[1] = u.x;
        result.m[5] = u.y;
        result.m[9] = u.z;
        result.m[2] = -f.x;
        result.m[6] = -f.y;
        result.m[10] = -f.z;

        result.m[12] = -Vec3::dot(r, eye);
        result.m[13] = -Vec3::dot(u, eye);
        result.m[14] = Vec3::dot(f, eye);

        return result;
    }

    static Mat4 translate(const Vec3& v) {
        Mat4 result;
        result.m[12] = v.x;
        result.m[13] = v.y;
        result.m[14] = v.z;
        return result;
    }

    static Mat4 scale(const Vec3& v) {
        Mat4 result;
        result.m[0] = v.x;
        result.m[5] = v.y;
        result.m[10] = v.z;
        return result;
    }

    static Mat4 rotateX(float angle) {
        Mat4 result;
        float c = std::cos(angle);
        float s = std::sin(angle);
        result.m[5] = c;
        result.m[9] = -s;
        result.m[6] = s;
        result.m[10] = c;
        return result;
    }

    static Mat4 rotateY(float angle) {
        Mat4 result;
        float c = std::cos(angle);
        float s = std::sin(angle);
        result.m[0] = c;
        result.m[8] = s;
        result.m[2] = -s;
        result.m[10] = c;
        return result;
    }

    static Mat4 rotateZ(float angle) {
        Mat4 result;
        float c = std::cos(angle);
        float s = std::sin(angle);
        result.m[0] = c;
        result.m[4] = -s;
        result.m[1] = s;
        result.m[5] = c;
        return result;
    }
};

// Axis-Aligned Bounding Box
struct AABB {
    Vec3 minPoint{ std::numeric_limits<float>::max() };
    Vec3 maxPoint{ std::numeric_limits<float>::lowest() };

    void expand(const Vec3& point) {
        minPoint = Vec3::minVec(minPoint, point);
        maxPoint = Vec3::maxVec(maxPoint, point);
    }

    Vec3 center() const {
        return (minPoint + maxPoint) * 0.5f;
    }
    Vec3 size() const {
        return maxPoint - minPoint;
    }
    float radius() const {
        return size().length() * 0.5f;
    }

    bool isValid() const {
        return minPoint.x <= maxPoint.x && minPoint.y <= maxPoint.y && minPoint.z <= maxPoint.z;
    }
};

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

inline float radians(float degrees) {
    return degrees * DEG_TO_RAD;
}
inline float degrees(float radians) {
    return radians * RAD_TO_DEG;
}
inline float clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

} // namespace Math
