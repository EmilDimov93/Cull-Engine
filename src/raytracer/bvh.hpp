// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

#include "../scene/scene.hpp"

#include <vector>
#include <limits>
#include <stdexcept>

namespace CL
{
    struct AABB
    {
        clm::vec3 min, max;

        AABB(const clm::vec3 &min, const clm::vec3 &max) : min(min), max(max) {}
        AABB() = default;

        bool contains(const clm::vec3 &point) const
        {
            return point.x >= min.x && point.y >= min.y && point.z >= min.z &&
                   point.x <= max.x && point.y <= max.y && point.z <= max.z;
        }

        bool intersects(const clm::vec3 &origin, const clm::vec3 &invDir) const
        {
            float tx1 = (min.x - origin.x) * invDir.x;
            float tx2 = (max.x - origin.x) * invDir.x;
            float tMin = std::min(tx1, tx2);
            float tMax = std::max(tx1, tx2);

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

    struct Node
    {
        AABB volume;

        std::vector<Vertex> vertices;
    };

    std::vector<Node> constructBVH(const std::vector<Model> &models, uint32_t levelCount)
    {
        if (levelCount == 0)
            throw std::runtime_error("Invalid BVH level count");

        clm::vec3 min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        clm::vec3 max(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

        for (const Model &model : models)
        {
            clm::mat4 mat = model.transform.mat();
            for (const Mesh &mesh : model.meshes)
            {
                for (const Vertex &v : mesh.vertices)
                {
                    clm::vec3 world = mat * v.pos;

                    if (min.x > world.x)
                        min.x = world.x;
                    if (min.y > world.y)
                        min.y = world.y;
                    if (min.z > world.z)
                        min.z = world.z;

                    if (max.x < world.x)
                        max.x = world.x;
                    if (max.y < world.y)
                        max.y = world.y;
                    if (max.z < world.z)
                        max.z = world.z;
                }
            }
        }

        std::vector<Node> nodes((1u << levelCount) - 1);

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

        for (uint32_t i = 0; i < levelCount - 1; i++)
        {
            for (uint32_t j = 0; j < (1u << i); j++)
            {
                subDivide((1u << i) - 1 + j);
            }
        }

        auto addTriangle = [&](auto &&self, const std::array<clm::vec3, 3> &pts, uint32_t i)
        {
            if (left(i) > nodes.size() - 1)
            {
                nodes[i].vertices.push_back(pts[0]);
                nodes[i].vertices.push_back(pts[1]);
                nodes[i].vertices.push_back(pts[2]);

                return;
            }

            if (nodes[left(i)].volume.contains(pts[0]) || nodes[left(i)].volume.contains(pts[1]) || nodes[left(i)].volume.contains(pts[2]))
            {
                self(self, pts, left(i));
            }

            if (nodes[right(i)].volume.contains(pts[0]) || nodes[right(i)].volume.contains(pts[1]) || nodes[right(i)].volume.contains(pts[2]))
            {
                self(self, pts, right(i));
            }
        };

        for (const Model &model : models)
        {
            clm::mat4 mat = model.transform.mat();
            for (const Mesh &mesh : model.meshes)
            {
                std::array<clm::vec3, 3> pts;
                for (uint32_t i = 0; i < mesh.indices.size(); i++)
                {
                    pts[i % 3] = mat * mesh.vertices[i].pos;
                    if (i % 3 == 2)
                    {
                        addTriangle(addTriangle, pts, 0);
                    }
                }
            }
        }

        return nodes;
    }
}