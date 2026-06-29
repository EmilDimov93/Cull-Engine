// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include <cmath>

namespace CL
{
    struct Mat4
    {
    private:
        float mat[4][4] = {{1, 0, 0, 0},
                           {0, 1, 0, 0},
                           {0, 0, 1, 0},
                           {0, 0, 0, 1}};

    public:
        using Column = float[4];

        Column &operator[](int col) { return mat[col]; }
        const Column &operator[](int col) const { return mat[col]; }

        Mat4 operator+(const Mat4 &other) const
        {
            Mat4 result;
            for (int col = 0; col < 4; ++col)
                for (int row = 0; row < 4; ++row)
                    result.mat[col][row] = mat[col][row] + other.mat[col][row];
            return result;
        };

        Mat4 operator-(const Mat4 &other) const
        {
            Mat4 result;
            for (int col = 0; col < 4; ++col)
                for (int row = 0; row < 4; ++row)
                    result.mat[col][row] = mat[col][row] - other.mat[col][row];
            return result;
        };

        Mat4 operator*(const Mat4 &other) const
        {
            Mat4 result;
            for (int col = 0; col < 4; ++col)
                for (int row = 0; row < 4; ++row)
                    result.mat[col][row] = mat[0][row] * other.mat[col][0] +
                                           mat[1][row] * other.mat[col][1] +
                                           mat[2][row] * other.mat[col][2] +
                                           mat[3][row] * other.mat[col][3];
            return result;
        };
    };

    inline Mat4 translationMat(float x, float y, float z)
    {
        Mat4 m;

        m[3][0] = x;
        m[3][1] = y;
        m[3][2] = z;

        return m;
    }

    inline Mat4 rotationMat(float x, float y, float z)
    {
        Mat4 m;

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

    inline Mat4 scaleMat(float x, float y, float z)
    {
        Mat4 m;

        m[0][0] = x;
        m[1][1] = y;
        m[2][2] = z;

        return m;
    }

    inline Mat4 scaleMat(float uniformScale)
    {
        return scaleMat(uniformScale, uniformScale, uniformScale);
    }
}