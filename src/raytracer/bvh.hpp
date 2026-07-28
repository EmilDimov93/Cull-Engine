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
    };

    struct Node
    {
        AABB volume;
    };

    std::vector<Node> constructBVH(const std::vector<Model> &models, uint32_t levelCount)
    {
        if(levelCount == 0)
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
            AABB full = nodes[index].volume;

            nodes[left(index)].volume = AABB(full.min, clm::vec3(full.min.x + (full.max.x - full.min.x) / 2, full.max.y, full.max.z));
            nodes[right(index)].volume = AABB(clm::vec3(full.min.x + (full.max.x - full.min.x) / 2, full.min.y, full.min.z), full.max);
        };

        for(uint32_t i = 0; i < levelCount - 1; i++)
        {
            for(uint32_t j = 0; j < (1u << i); j++)
            {
                subDivide((1u << i) - 1 + j);
            }
        }

        return nodes;
    }
}