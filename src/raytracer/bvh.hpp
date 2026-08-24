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

        void expand(const clm::vec3 &point)
        {
            min.x = std::min(min.x, point.x);
            min.y = std::min(min.y, point.y);
            min.z = std::min(min.z, point.z);
            max.x = std::max(max.x, point.x);
            max.y = std::max(max.y, point.y);
            max.z = std::max(max.z, point.z);
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

        void buildRecursive(uint32_t currNodeIndex = 0)
        {
            AABB centroidBox;

            for (const Triangle &triangle : nodes[currNodeIndex].triangles)
            {
                nodes[currNodeIndex].volume.expand(triangle.vertices[0].pos);
                nodes[currNodeIndex].volume.expand(triangle.vertices[1].pos);
                nodes[currNodeIndex].volume.expand(triangle.vertices[2].pos);

                centroidBox.expand(triangle.centroid());
            }

            if (nodes[currNodeIndex].triangles.size() <= 2)
                return;

            float x = fabsf(centroidBox.max.x - centroidBox.min.x);
            float y = fabsf(centroidBox.max.y - centroidBox.min.y);
            float z = fabsf(centroidBox.max.z - centroidBox.min.z);

            auto midPoint = [](float min, float max)
            {
                return min + (max - min) / 2;
            };

            std::vector<Triangle> leftTris, rightTris;

            if (x > y && x > z)
            {
                float mid = midPoint(centroidBox.min.x, centroidBox.max.x);

                for (const Triangle &triangle : nodes[currNodeIndex].triangles)
                {
                    if (triangle.centroid().x < mid)
                        leftTris.push_back(triangle);
                    else
                        rightTris.push_back(triangle);
                }
            }
            else if (y > x && y > z)
            {
                float mid = midPoint(centroidBox.min.y, centroidBox.max.y);

                for (const Triangle &triangle : nodes[currNodeIndex].triangles)
                {
                    if (triangle.centroid().y < mid)
                        leftTris.push_back(triangle);
                    else
                        rightTris.push_back(triangle);
                }
            }
            else
            {
                float mid = midPoint(centroidBox.min.z, centroidBox.max.z);

                for (const Triangle &triangle : nodes[currNodeIndex].triangles)
                {
                    if (triangle.centroid().z < mid)
                        leftTris.push_back(triangle);
                    else
                        rightTris.push_back(triangle);
                }
            }

            if (leftTris.empty() || rightTris.empty())
                return;

            const uint32_t leftIndex = static_cast<uint32_t>(nodes.size());
            const uint32_t rightIndex = leftIndex + 1;
            nodes.emplace_back();
            nodes.emplace_back();
            nodes[leftIndex].triangles = std::move(leftTris);
            nodes[leftIndex + 1].triangles = std::move(rightTris);
            nodes[currNodeIndex].leftIndex = leftIndex;

            nodes[currNodeIndex].triangles.clear();
            nodes[currNodeIndex].triangles.shrink_to_fit();

            buildRecursive(leftIndex);
            buildRecursive(rightIndex);
        }

        BVH(const std::vector<Model> &models)
        {
            std::vector<Triangle> triangles;

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
                            triangles.emplace_back(vertices, startMaterialIndex + mesh.materialIndex);
                    }
                }

                materials.insert(materials.end(), model.materials.begin(), model.materials.end());
                startMaterialIndex = materials.size();
            }

            Node root;
            nodes.push_back(root);
            nodes[0].triangles = triangles;

            buildRecursive();
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

                uint32_t leftIndex = nodes[index].leftIndex;

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