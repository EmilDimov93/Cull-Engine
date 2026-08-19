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
        AABB() = default;

        bool intersectsTriangle(const std::array<Vertex, 3> &triangle) const
        {
            const clm::vec3 boxCenter = (min + max) * 0.5f;
            const clm::vec3 halfExtents = (max - min) * 0.5f;

            const clm::vec3 v0 = triangle[0].pos - boxCenter;
            const clm::vec3 v1 = triangle[1].pos - boxCenter;
            const clm::vec3 v2 = triangle[2].pos - boxCenter;

            if (std::min({v0.x, v1.x, v2.x}) > halfExtents.x || std::max({v0.x, v1.x, v2.x}) < -halfExtents.x)
                return false;
            if (std::min({v0.y, v1.y, v2.y}) > halfExtents.y || std::max({v0.y, v1.y, v2.y}) < -halfExtents.y)
                return false;
            if (std::min({v0.z, v1.z, v2.z}) > halfExtents.z || std::max({v0.z, v1.z, v2.z}) < -halfExtents.z)
                return false;

            const clm::vec3 edge0 = v1 - v0;
            const clm::vec3 edge1 = v2 - v1;
            const clm::vec3 edge2 = v0 - v2;

            {
                const clm::vec3 normal = edge0.cross(edge1);
                const float boxRadius =
                    halfExtents.x * std::abs(normal.x) +
                    halfExtents.y * std::abs(normal.y) +
                    halfExtents.z * std::abs(normal.z);
                if (std::abs(normal.dot(v0)) > boxRadius)
                    return false;
            }

            const clm::vec3 edges[3] = {edge0, edge1, edge2};
            const clm::vec3 boxAxes[3] = {
                clm::vec3(1.0f, 0.0f, 0.0f),
                clm::vec3(0.0f, 1.0f, 0.0f),
                clm::vec3(0.0f, 0.0f, 1.0f)};

            for (const clm::vec3 &edge : edges)
            {
                for (const clm::vec3 &boxAxis : boxAxes)
                {
                    const clm::vec3 axis = edge.cross(boxAxis);

                    const float projection0 = axis.dot(v0);
                    const float projection1 = axis.dot(v1);
                    const float projection2 = axis.dot(v2);

                    const float triangleMin = std::min({projection0, projection1, projection2});
                    const float triangleMax = std::max({projection0, projection1, projection2});

                    const float boxRadius =
                        halfExtents.x * std::abs(axis.x) +
                        halfExtents.y * std::abs(axis.y) +
                        halfExtents.z * std::abs(axis.z);

                    if (triangleMin > boxRadius || triangleMax < -boxRadius)
                        return false;
                }
            }

            return true;
        }

        bool intersectsRay(const clm::vec3 &origin, const clm::vec3 &invDir) const
        {
            float tMin = std::numeric_limits<float>::lowest();
            float tMax = std::numeric_limits<float>::max();

            float tx1 = (min.x - origin.x) * invDir.x;
            float tx2 = (max.x - origin.x) * invDir.x;
            tMin = std::max(tMin, std::min(tx1, tx2));
            tMax = std::min(tMax, std::max(tx1, tx2));

            float ty1 = (min.y - origin.y) * invDir.y;
            float ty2 = (max.y - origin.y) * invDir.y;
            tMin = std::max(tMin, std::min(ty1, ty2));
            tMax = std::min(tMax, std::max(ty1, ty2));

            float tz1 = (min.z - origin.z) * invDir.z;
            float tz2 = (max.z - origin.z) * invDir.z;
            tMin = std::max(tMin, std::min(tz1, tz2));
            tMax = std::min(tMax, std::max(tz1, tz2));

            return tMax >= std::max(tMin, 0.0f);
        }
    };

    struct Triangle
    {
        std::array<Vertex, 3> vertices;
        uint32_t material;

        Triangle() = default;
        Triangle(std::array<Vertex, 3> vertices, uint32_t material) : vertices(vertices), material(material) {}
    };

    struct Node
    {
        AABB volume;
        std::vector<Triangle> triangles;
    };

    struct BVH
    {
        std::vector<Node> nodes;
        std::vector<Material> materials;

        BVH(const std::vector<Model> &models, uint32_t depth)
        {
            if (depth == 0)
                throw std::runtime_error("Invalid BVH depth");

            clm::vec3 min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
            clm::vec3 max(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

            static constexpr float EPSILON = 1e-4f;

            for (const Model &model : models)
            {
                clm::mat4 mat = model.transform.mat();
                for (const Mesh &mesh : model.meshes)
                {
                    for (const Vertex &v : mesh.vertices)
                    {
                        clm::vec3 world = (mat * clm::vec4(v.pos, 1.f)).xyz();

                        if (min.x > world.x - EPSILON)
                            min.x = world.x - EPSILON;
                        if (min.y > world.y - EPSILON)
                            min.y = world.y - EPSILON;
                        if (min.z > world.z - EPSILON)
                            min.z = world.z - EPSILON;

                        if (max.x < world.x + EPSILON)
                            max.x = world.x + EPSILON;
                        if (max.y < world.y + EPSILON)
                            max.y = world.y + EPSILON;
                        if (max.z < world.z + EPSILON)
                            max.z = world.z + EPSILON;
                    }
                }
            }

            nodes.resize((1u << depth) - 1);

            nodes[0].volume = AABB(min, max);

            auto left = [](uint32_t index)
            {
                return 2 * index + 1;
            };

            auto right = [](uint32_t index)
            {
                return 2 * index + 2;
            };

            auto subDivide = [&](uint32_t index)
            {
                const AABB &full = nodes[index].volume;

                float xLen = full.max.x - full.min.x;
                float yLen = full.max.y - full.min.y;
                float zLen = full.max.z - full.min.z;

                auto midPoint = [](float min, float max)
                {
                    return min + (max - min) / 2;
                };

                clm::vec3 leftMax = full.max;
                clm::vec3 rightMin = full.min;

                if (xLen >= yLen && xLen >= zLen)
                {
                    const float mid = midPoint(full.min.x, full.max.x);
                    leftMax.x = mid;
                    rightMin.x = mid;
                }
                else if (yLen >= xLen && yLen >= zLen)
                {
                    const float mid = midPoint(full.min.y, full.max.y);
                    leftMax.y = mid;
                    rightMin.y = mid;
                }
                else
                {
                    const float mid = midPoint(full.min.z, full.max.z);
                    leftMax.z = mid;
                    rightMin.z = mid;
                }

                nodes[left(index)].volume = AABB(full.min, leftMax);
                nodes[right(index)].volume = AABB(rightMin, full.max);
            };

            for (uint32_t i = 0; i < depth - 1; i++)
            {
                for (uint32_t j = 0; j < (1u << i); j++)
                {
                    subDivide((1u << i) - 1 + j);
                }
            }

            auto addTriangle = [&](auto &&self, const std::array<Vertex, 3> &vertices, uint32_t materialIndex, uint32_t i)
            {
                if (left(i) > nodes.size() - 1)
                {
                    nodes[i].triangles.emplace_back(vertices, materialIndex);

                    return;
                }

                if (nodes[left(i)].volume.intersectsTriangle(vertices))
                {
                    self(self, vertices, materialIndex, left(i));
                }

                if (nodes[right(i)].volume.intersectsTriangle(vertices))
                {
                    self(self, vertices, materialIndex, right(i));
                }
            };

            uint32_t startMaterialIndex = 0;
            for (const Model &model : models)
            {
                clm::mat4 mat = model.transform.mat();
                for (const Mesh &mesh : model.meshes)
                {
                    std::array<Vertex, 3> vertices;
                    for (uint32_t i = 0; i < mesh.indices.size(); i++)
                    {
                        vertices[i % 3].pos = (mat * clm::vec4(mesh.vertices[mesh.indices[i]].pos, 1.f)).xyz();
                        vertices[i % 3].normal = model.transform.rot * mesh.vertices[mesh.indices[i]].normal;
                        vertices[i % 3].tex = mesh.vertices[mesh.indices[i]].tex;
                        if (i % 3 == 2)
                            addTriangle(addTriangle, vertices, startMaterialIndex + mesh.materialIndex, 0);
                    }
                }

                materials.insert(materials.end(), model.materials.begin(), model.materials.end());
                startMaterialIndex = materials.size();
            }
        }

        std::vector<uint32_t> getHitLeaves(const clm::vec3 &rayOrigin, const clm::vec3 &rayDir) const
        {
            std::vector<uint32_t> hitLeaves;

            uint32_t stack[64];
            int stackPointer = 0;
            stack[stackPointer++] = 0;

            while (stackPointer > 0)
            {
                uint32_t index = stack[--stackPointer];

                if (!nodes[index].volume.intersectsRay(rayOrigin, clm::vec3(1.f / rayDir.x, 1.f / rayDir.y, 1.f / rayDir.z)))
                    continue;

                uint32_t leftIndex = 2 * index + 1;

                if (leftIndex > nodes.size() - 1)
                {
                    hitLeaves.push_back(index);
                    continue;
                }

                stack[stackPointer++] = leftIndex;
                stack[stackPointer++] = leftIndex + 1;
            }

            return hitLeaves;
        }
    };
}