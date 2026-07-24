// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

#include "../scene/scene.hpp"

#include <vector>

namespace CL
{
    inline constexpr void placePixel3c(clm::vec4 color, uint32_t x, uint32_t y, std::vector<uint8_t> &image, uint32_t imageWidth)
    {
        if (x >= imageWidth || y >= image.size() / imageWidth)
            return;

        image[y * imageWidth * 3 + x * 3] = static_cast<uint8_t>(color.x);
        image[y * imageWidth * 3 + x * 3 + 1] = static_cast<uint8_t>(color.y);
        image[y * imageWidth * 3 + x * 3 + 2] = static_cast<uint8_t>(color.z);
    }

    void castRay(const std::vector<Model> &models, const clm::vec3 &rayOrigin, const clm::vec3 &rayDirection, uint32_t *outModelIndex, uint32_t *outMeshIndex, clm::vec3 *outNormal, clm::vec3 *outIntersectionPoint);

    void renderSceneToPPM(clm::uvec2 imageSize, const Scene &scene, float fov, float vignetteStrength);
}