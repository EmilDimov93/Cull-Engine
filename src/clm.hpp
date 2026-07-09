// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include <cmath>

namespace clm
{
    static constexpr float PI = 3.14159265f;

    struct vec3
    {
        float x, y, z;

        constexpr vec3(float x = 0.f, float y = 0.f, float z = 0.f) : x(x), y(y), z(z) {}

        [[nodiscard]] constexpr vec3 operator*(const vec3 &other) const
        {
            return vec3(x * other.x, y * other.y, z * other.z);
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

        [[nodiscard]] constexpr vec4 operator+(const vec4 &other) const
        {
            return vec4(x + other.x, y + other.y, z + other.z, w + other.w);
        }

        [[nodiscard]] constexpr vec4 operator*(float scalar) const
        {
            return vec4(x * scalar, y * scalar, z * scalar, w * scalar);
        }
    };

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
    };

    [[nodiscard]] constexpr inline mat4 translationMat(float x, float y, float z)
    {
        mat4 m;

        m[3][0] = x;
        m[3][1] = y;
        m[3][2] = z;

        return m;
    }

    [[nodiscard]] constexpr inline mat4 translationMat(vec3 translation)
    {
        return translationMat(translation.x, translation.y, translation.z);
    }

    [[nodiscard]] constexpr inline mat4 rotationMat(float x, float y, float z)
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

    [[nodiscard]] constexpr inline mat4 scaleMat(float x, float y, float z)
    {
        mat4 m;

        m[0][0] = x;
        m[1][1] = y;
        m[2][2] = z;

        return m;
    }

    [[nodiscard]] constexpr inline mat4 scaleMat(float uniformScale)
    {
        return scaleMat(uniformScale, uniformScale, uniformScale);
    }

    inline float constexpr unitToSignedRange(float unitValue)
    {
        return unitValue * 2.f - 1.f;
    }

    inline float constexpr signedToUnitRange(float signedValue)
    {
        return (signedValue + 1.f) / 2.f;
    }
}