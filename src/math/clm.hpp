// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include <cmath>
#include <cstdint>

namespace clm
{
    inline constexpr float PI = 3.14159265f;

    struct ivec2
    {
        int32_t x, y;

        constexpr ivec2(int32_t x = 0, int32_t y = 0) : x(x), y(y) {}
    };

    struct uvec2
    {
        uint32_t x, y;

        constexpr uvec2(uint32_t x = 0u, uint32_t y = 0u) : x(x), y(y) {}
    };

    struct vec2
    {
        float x, y;

        constexpr vec2(float x = 0.f, float y = 0.f) : x(x), y(y) {}
    };

    struct vec3
    {
        float x, y, z;

        constexpr vec3(float x = 0.f, float y = 0.f, float z = 0.f) : x(x), y(y), z(z) {}

        [[nodiscard]] constexpr vec3 operator*(const vec3 &other) const
        {
            return vec3(x * other.x, y * other.y, z * other.z);
        }

        [[nodiscard]] constexpr vec3 operator/(const vec3 &other) const
        {
            return vec3(x / other.x, y / other.y, z / other.z);
        }

        [[nodiscard]] constexpr vec3 operator+(const vec3 &other) const
        {
            return vec3(x + other.x, y + other.y, z + other.z);
        }

        [[nodiscard]] constexpr vec3 operator-(const vec3 &other) const
        {
            return vec3(x - other.x, y - other.y, z - other.z);
        }

        void constexpr operator+=(const vec3 &other)
        {
            x += other.x;
            y += other.y;
            z += other.z;
        }

        [[nodiscard]] vec3 constexpr operator*(float scalar) const
        {
            return vec3(x * scalar, y * scalar, z * scalar);
        }

        [[nodiscard]] vec3 constexpr operator/(float scalar) const
        {
            return vec3(x / scalar, y / scalar, z / scalar);
        }

        [[nodiscard]] constexpr float dot(vec3 other) const
        {
            return x * other.x + y * other.y + z * other.z;
        }

        [[nodiscard]] constexpr vec3 cross(vec3 other) const
        {
            return vec3(y * other.z - z * other.y,
                        z * other.x - x * other.z,
                        x * other.y - y * other.x);
        }

        [[nodiscard]] constexpr float length() const
        {
            return std::sqrt(x * x + y * y + z * z);
        }

        [[nodiscard]] constexpr vec3 normalized() const
        {
            float len = length();
            return vec3(x / len, y / len, z / len);
        }
    };

    struct vec4
    {
        float x, y, z, w;

        constexpr vec4(float x = 0.f, float y = 0.f, float z = 0.f, float w = 0.f) : x(x), y(y), z(z), w(w) {}
        constexpr vec4(vec3 v3, float w) : x(v3.x), y(v3.y), z(v3.z), w(w) {}

        [[nodiscard]] constexpr vec4 operator+(const vec4 &other) const
        {
            return vec4(x + other.x, y + other.y, z + other.z, w + other.w);
        }

        [[nodiscard]] constexpr vec4 operator*(float scalar) const
        {
            return vec4(x * scalar, y * scalar, z * scalar, w * scalar);
        }

        [[nodiscard]] constexpr vec4 operator/(float scalar) const
        {
            return vec4(x / scalar, y / scalar, z / scalar, w / scalar);
        }
    };

    [[nodiscard]] constexpr float clamp(float value, float min, float max)
    {
        return value < min ? min : (max < value ? max : value);
    }

    [[nodiscard]] constexpr clm::vec4 clamp(const clm::vec4 &v, float min, float max)
    {
        return clm::vec4(clamp(v.x, min, max),
                         clamp(v.y, min, max),
                         clamp(v.z, min, max),
                         clamp(v.w, min, max));
    }

    struct mat4
    {
    private:
        float mat[4][4] = {{1, 0, 0, 0},
                           {0, 1, 0, 0},
                           {0, 0, 1, 0},
                           {0, 0, 0, 1}};

    public:
        using Column = float[4];

        constexpr Column &operator[](int col) { return mat[col]; }
        constexpr const Column &operator[](int col) const { return mat[col]; }

        [[nodiscard]] constexpr mat4 operator+(const mat4 &other) const
        {
            mat4 result;
            for (int col = 0; col < 4; ++col)
                for (int row = 0; row < 4; ++row)
                    result.mat[col][row] = mat[col][row] + other.mat[col][row];
            return result;
        };

        [[nodiscard]] constexpr mat4 operator-(const mat4 &other) const
        {
            mat4 result;
            for (int col = 0; col < 4; ++col)
                for (int row = 0; row < 4; ++row)
                    result.mat[col][row] = mat[col][row] - other.mat[col][row];
            return result;
        };

        [[nodiscard]] constexpr mat4 operator*(const mat4 &other) const
        {
            mat4 result;
            for (int col = 0; col < 4; ++col)
                for (int row = 0; row < 4; ++row)
                    result.mat[col][row] = mat[0][row] * other.mat[col][0] +
                                           mat[1][row] * other.mat[col][1] +
                                           mat[2][row] * other.mat[col][2] +
                                           mat[3][row] * other.mat[col][3];
            return result;
        };

        [[nodiscard]] constexpr vec3 operator*(const vec3 &v) const
        {
            return vec3(mat[0][0] * v.x + mat[1][0] * v.y + mat[2][0] * v.z + mat[3][0],
                        mat[0][1] * v.x + mat[1][1] * v.y + mat[2][1] * v.z + mat[3][1],
                        mat[0][2] * v.x + mat[1][2] * v.y + mat[2][2] * v.z + mat[3][2]);
        }

        [[nodiscard]] constexpr vec4 operator*(const vec4 &v) const
        {
            return vec4(mat[0][0] * v.x + mat[1][0] * v.y + mat[2][0] * v.z + mat[3][0] * v.w,
                        mat[0][1] * v.x + mat[1][1] * v.y + mat[2][1] * v.z + mat[3][1] * v.w,
                        mat[0][2] * v.x + mat[1][2] * v.y + mat[2][2] * v.z + mat[3][2] * v.w,
                        mat[0][3] * v.x + mat[1][3] * v.y + mat[2][3] * v.z + mat[3][3] * v.w);
        }

        [[nodiscard]] static constexpr inline mat4 translation(float x, float y, float z)
        {
            mat4 m;

            m[3][0] = x;
            m[3][1] = y;
            m[3][2] = z;

            return m;
        }

        [[nodiscard]] static constexpr inline mat4 translation(vec3 t)
        {
            return translation(t.x, t.y, t.z);
        }

        [[nodiscard]] static constexpr inline mat4 rotation(float x, float y, float z)
        {
            mat4 m;

            float cosX = std::cos(x);
            float sinX = std::sin(x);
            float cosY = std::cos(y);
            float sinY = std::sin(y);
            float cosZ = std::cos(z);
            float sinZ = std::sin(z);

            m[0][0] = cosY * cosZ + sinY * sinX * sinZ;
            m[1][0] = -cosY * sinZ + sinY * sinX * cosZ;
            m[2][0] = sinY * cosX;

            m[0][1] = cosX * sinZ;
            m[1][1] = cosX * cosZ;
            m[2][1] = -sinX;

            m[0][2] = -sinY * cosZ + cosY * sinX * sinZ;
            m[1][2] = sinY * sinZ + cosY * sinX * cosZ;
            m[2][2] = cosY * cosX;

            return m;
        }

        [[nodiscard]] static constexpr inline mat4 scale(float x, float y, float z)
        {
            mat4 m;

            m[0][0] = x;
            m[1][1] = y;
            m[2][2] = z;

            return m;
        }

        [[nodiscard]] static constexpr inline mat4 scale(float uniformScale)
        {
            return scale(uniformScale, uniformScale, uniformScale);
        }

        [[nodiscard]] static constexpr inline mat4 perspective(float fieldOfView, float aspectRatio, float zNear, float zFar)
        {
            mat4 m;

            const float f = 1.0f / std::tan(fieldOfView / 2.f);

            m[0][0] = f / aspectRatio;
            m[1][1] = f;
            m[2][2] = (zFar + zNear) / (zNear - zFar);
            m[3][2] = (2.f * zFar * zNear) / (zNear - zFar);
            m[2][3] = 1.f;
            m[3][3] = 0.f;

            return m;
        }
    };

    constexpr float unitToSignedRange(float unitValue)
    {
        return unitValue * 2.f - 1.f;
    }

    constexpr float signedToUnitRange(float signedValue)
    {
        return (signedValue + 1.f) / 2.f;
    }

    struct quaternion
    {
        float w, x, y, z;

        constexpr quaternion(float w = 1.f, float x = 0.f, float y = 0.f, float z = 0.f) : w(w), x(x), y(y), z(z) {}
        constexpr quaternion(vec3 rot) : quaternion() { *this = rotate(rot); }

        [[nodiscard]] constexpr quaternion operator*(const quaternion &other) const
        {
            const vec4 A(x, y, z, w);
            const vec4 B(other.x, other.y, other.z, other.w);

            return quaternion(-A.x * B.x + -A.y * B.y + -A.z * B.z + A.w * B.w,
                              A.x * B.w + A.y * B.z + -A.z * B.y + A.w * B.x,
                              -A.x * B.z + A.y * B.w + A.z * B.x + A.w * B.y,
                              A.x * B.y + -A.y * B.x + A.z * B.w + A.w * B.z);
        }

        [[nodiscard]] constexpr quaternion normalized() const
        {
            const float length = std::sqrt(w * w + x * x + y * y + z * z);

            return quaternion(w / length, x / length, y / length, z / length);
        }

        [[nodiscard]] constexpr quaternion conjugate() const
        {
            return quaternion(w, -x, -y, -z);
        }

        [[nodiscard]] constexpr quaternion rotate(vec3 rotation) const
        {
            const float halfX = rotation.x / 2.f;
            const float halfY = rotation.y / 2.f;
            const float halfZ = rotation.z / 2.f;

            const float cosX = std::cos(halfX);
            const float sinX = std::sin(halfX);
            const float cosY = std::cos(halfY);
            const float sinY = std::sin(halfY);
            const float cosZ = std::cos(halfZ);
            const float sinZ = std::sin(halfZ);

            const quaternion delta(
                cosX * cosY * cosZ + sinX * sinY * sinZ,
                sinX * cosY * cosZ - cosX * sinY * sinZ,
                cosX * sinY * cosZ + sinX * cosY * sinZ,
                cosX * cosY * sinZ - sinX * sinY * cosZ);

            return (delta * (*this)).normalized();
        }

        [[nodiscard]] constexpr mat4 mat() const
        {
            const float xx = x * x, yy = y * y, zz = z * z;
            const float xy = x * y, xz = x * z, yz = y * z;
            const float wx = w * x, wy = w * y, wz = w * z;

            mat4 result;

            result[0][0] = 1.f - 2.f * (yy + zz);
            result[0][1] = 2.f * (xy + wz);
            result[0][2] = 2.f * (xz - wy);

            result[1][0] = 2.f * (xy - wz);
            result[1][1] = 1.f - 2.f * (xx + zz);
            result[1][2] = 2.f * (yz + wx);

            result[2][0] = 2.f * (xz + wy);
            result[2][1] = 2.f * (yz - wx);
            result[2][2] = 1.f - 2.f * (xx + yy);

            return result;
        }
    };
}