// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include <vector>
#include <array>
#include <cstdint>

namespace CL
{
    struct Vec3
    {
        float x, y, z;

        Vec3(float x = 0.f, float y = 0.f, float z = 0.f) : x(x), y(y), z(z) {}
    };

    struct Vec4
    {
        float x, y, z, w;

        Vec4(float x = 0.f, float y = 0.f, float z = 0.f, float w = 0.f) : x(x), y(y), z(z), w(w) {}
    };

    using Mat4 = std::array<Vec4, 4>;

    struct Vertex
    {
        float x, y, z;

        Vertex(float x = 0.f, float y = 0.f, float z = 0.f) : x(x), y(y), z(z) {}
    };

    struct Material
    {
        Vec4 color;

        Material(Vec4 color) : color(color) {}
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        uint32_t materialIndex;

        Mesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> indices, uint32_t materialIndex) : vertices(vertices), indices(indices), materialIndex(materialIndex) {}
    };

    struct Model
    {
        std::vector<Mesh> meshes;
        std::vector<Material> materials;

        Mat4 modelMat;

        Model(const std::vector<Mesh> &meshes, const std::vector<Material> &materials, Mat4 modelMat) : meshes(meshes), materials(materials), modelMat(modelMat) {}
    };
}
