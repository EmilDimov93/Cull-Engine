// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

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
}