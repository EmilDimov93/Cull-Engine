// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

#include "../scene/scene.hpp"

#include "../attachments.hpp"

#include <vector>

namespace CL
{
    struct Ray
    {
        clm::vec3 origin;
        clm::vec3 direction;

        Ray(const clm::vec3 &origin, const clm::vec3 &direction) : origin(origin), direction(direction) {}
    };

    class RayGenerator
    {
    public:
        RayGenerator(clm::uvec2 imageSize, const Camera &camera, float fov) : imageSize(imageSize), aspectRatio(static_cast<float>(imageSize.x) / imageSize.y), tanHalfFov(std::tan(fov / 2.f)), origin(camera.getPos()), basis(camera.getBasis()) {}

        Ray generateRay(const clm::uvec2 &pixel) const
        {
            const clm::vec2 ndc(clm::unitToSignedRange((pixel.x + 0.5f) / imageSize.x) * aspectRatio * tanHalfFov,
                                -clm::unitToSignedRange((pixel.y + 0.5f) / imageSize.y) * tanHalfFov);

            return Ray(origin,
                       (basis.right * ndc.x + basis.up * ndc.y + basis.forward).normalized());
        }

    private:
        clm::uvec2 imageSize;
        float aspectRatio;
        float tanHalfFov;
        clm::vec3 origin;
        Camera::Basis basis;
    };

    [[nodiscard]] uint32_t castRay(const std::vector<Model> &models, const Ray &ray);

    ColorAttachment renderSceneRayTraced(clm::uvec2 imageSize, const Scene &scene, float fov, float vignetteStrength, uint32_t bvhDepth);
}