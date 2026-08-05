// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

#include "../scene/scene.hpp"

#include "../attachments.hpp"

#include <vector>

namespace CL
{
    void castRay(const std::vector<Model> &models, const clm::vec3 &rayOrigin, const clm::vec3 &rayDirection, uint32_t *outModelIndex, uint32_t *outMeshIndex, clm::vec3 *outNormal, clm::vec3 *outIntersectionPoint);

    ColorAttachment renderSceneRayTraced(clm::uvec2 imageSize, const Scene &scene, float fov, float vignetteStrength, uint32_t bvhDepth);

    void exportAttachmentPPM(const ColorAttachment &colorAtt, const std::string &filePath, bool shouldOpen);
}