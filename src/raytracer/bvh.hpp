// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

#include "../scene/scene.hpp"

#include <vector>
#include <limits>
#include <stdexcept>
#include <array>
#include <algorithm>

namespace CL
{
    struct AABB
    {
        clm::vec3 min, max;

        AABB(const clm::vec3 &min, const clm::vec3 &max) : min(min), max(max) {}
        AABB() : min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::lowest()) {}

        void expand(const clm::vec3 &point);

        bool intersectsRay(const clm::vec3 &origin, const clm::vec3 &invDir) const;
    };

    struct Triangle
    {
        std::array<Vertex, 3> vertices;
        uint32_t material;

        Triangle() = default;
        Triangle(std::array<Vertex, 3> vertices, uint32_t material) : vertices(vertices), material(material) {}

        clm::vec3 centroid() const
        {
            return (vertices[0].pos + vertices[1].pos + vertices[2].pos) / 3.f;
        }
    };

    struct Node
    {
        AABB volume;
        std::vector<Triangle> triangles;
        uint32_t leftIndex = INVALID_INDEX;
    };

    struct BVH
    {
        std::vector<Node> nodes;
        std::vector<Material> materials;

        BVH(const std::vector<Model> &models);
        void buildRecursive(uint32_t currNodeIndex = 0);
        std::vector<uint32_t> getHitLeaves(const clm::vec3 &rayOrigin, const clm::vec3 &rayDir) const;
    };
}