// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include <vector>
#include <cstdint>

namespace CL
{
    struct Vec3
    {
        float x, y, z;

        Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    };

    struct Vec4
    {
        float x, y, z, w;

        Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    };

    struct Vertex
    {
        float x, y, z;

        Vertex(float x, float y, float z) : x(x), y(y), z(z) {}
    };

    struct Material
    {
        Vec4 color;
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };
}
