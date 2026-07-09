// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

#include <thread>

namespace CL
{
    [[nodiscard]] bool RayIntersectsTriangle(const clm::vec3 &rayOrigin, const clm::vec3 &rayVector, const std::array<clm::vec3, 3> &pts, clm::vec3 &outIntersectionPoint)
    {
        constexpr float EPSILON = 1e-7f;
        constexpr float RAY_MIN_DISTANCE = 1e-3f;

        const clm::vec3 edge01 = pts[1] - pts[0];
        const clm::vec3 edge02 = pts[2] - pts[0];

        const clm::vec3 rayCrossEdge02 = rayVector.cross(edge02);

        float determinant = edge01.dot(rayCrossEdge02);

        if (determinant > -EPSILON && determinant < EPSILON)
            return false;

        const float inverseDeterminant = 1 / determinant;
        const clm::vec3 vertex0ToRayOrigin = rayOrigin - pts[0];
        const float barycentricU = inverseDeterminant * vertex0ToRayOrigin.dot(rayCrossEdge02);

        if (barycentricU < 0.0 || barycentricU > 1.0)
            return false;

        const clm::vec3 originCrossEdge01 = vertex0ToRayOrigin.cross(edge01);

        const float barycentricV = inverseDeterminant * rayVector.dot(originCrossEdge01);

        if (barycentricV < 0.0 || barycentricU + barycentricV > 1.0)
            return false;

        const float rayDistance = inverseDeterminant * edge02.dot(originCrossEdge01);

        if (rayDistance > RAY_MIN_DISTANCE)
        {
            outIntersectionPoint = rayOrigin + (rayVector.normalized() * (rayDistance * rayVector.length()));
            return true;
        }
        else
        {
            return false;
        }
    }

    void Renderer::castRay(const std::vector<Model> &models, const clm::vec3 &rayOrigin, const clm::vec3 &rayDirection, uint32_t *outModelIndex, uint32_t *outMeshIndex, clm::vec3 *outNormal, clm::vec3 *outIntersectionPoint)
    {
        float closestDistance = std::numeric_limits<float>::max();

        for (uint32_t i = 0; i < models.size(); i++)
        {
            const clm::mat4 modelMat = models[i].transform.mat();
            for (uint32_t j = 0; j < models[i].meshes.size(); j++)
            {
                const Mesh &mesh = models[i].meshes[j];
                for (uint32_t indexOffset = 0; indexOffset + 2 < mesh.indices.size(); indexOffset += 3)
                {
                    const std::array<clm::vec3, 3> pts = {
                        modelMat * mesh.vertices[mesh.indices[indexOffset + 0]].pos,
                        modelMat * mesh.vertices[mesh.indices[indexOffset + 1]].pos,
                        modelMat * mesh.vertices[mesh.indices[indexOffset + 2]].pos};

                    clm::vec3 intersectionPoint;
                    if (RayIntersectsTriangle(rayOrigin, rayDirection, pts, intersectionPoint))
                    {
                        const float distance = (intersectionPoint - rayOrigin).length();
                        if (distance < closestDistance)
                        {
                            closestDistance = distance;

                            if (outModelIndex)
                                *outModelIndex = i;
                            if (outMeshIndex)
                                *outMeshIndex = j;

                            if (outNormal)
                            {
                                clm::vec3 edge1 = pts[1] - pts[0];
                                clm::vec3 edge2 = pts[2] - pts[0];
                                *outNormal = edge1.cross(edge2).normalized();
                            }

                            if(outIntersectionPoint)
                                *outIntersectionPoint = intersectionPoint;
                        }
                    }
                }
            }
        }
    }

    const std::vector<uint8_t> Renderer::getImageRayTraced(clm::vec2 imageSize)
    {
        std::vector<uint8_t> image(static_cast<uint32_t>(imageSize.x) * static_cast<uint32_t>(imageSize.y) * 3);

        const unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
        std::vector<std::thread> workers;
        workers.reserve(threadCount);

        const clm::vec3 forward = {cosf(cameraRot.x) * sinf(cameraRot.y), -sinf(cameraRot.x), cosf(cameraRot.x) * cosf(cameraRot.y)};
        const clm::vec3 right = {cosf(cameraRot.y), 0.f, -sinf(cameraRot.y)};
        const clm::vec3 up = {sinf(cameraRot.x) * sinf(cameraRot.y), cosf(cameraRot.x), sinf(cameraRot.x) * cosf(cameraRot.y)};

        auto renderRows = [&](unsigned int threadIndex)
        {
            for (uint32_t pixelY = threadIndex; pixelY < imageSize.y; pixelY += threadCount)
            {
                for (uint32_t pixelX = 0; pixelX < imageSize.x; pixelX++)
                {
                    const float aspectRatio = imageSize.x / imageSize.y;
                    const float ndcX = clm::unitToSignedRange((pixelX + 0.5f) / imageSize.x) * aspectRatio * TAN_HALF_FOV;
                    const float ndcY = -clm::unitToSignedRange((pixelY + 0.5f) / imageSize.y) * TAN_HALF_FOV;

                    const clm::vec3 rayDirection = (right * ndcX + up * ndcY + forward).normalized();

                    clm::vec4 pixelColor = {clearColor.x, clearColor.y, clearColor.z, 1.f};

                    const float smallestDot = clm::vec3(clm::unitToSignedRange(0.5f / imageSize.x) * aspectRatio * TAN_HALF_FOV, -clm::unitToSignedRange(0.5f / imageSize.y) * TAN_HALF_FOV, 1.f).normalized().dot(clm::vec3(0.f, 0.f, 1.f));
                    const float biggestDot = clm::vec3(clm::unitToSignedRange((imageSize.x / 2.f + 0.5f) / imageSize.x) * aspectRatio * TAN_HALF_FOV, -clm::unitToSignedRange((imageSize.y / 2.f + 0.5f) / imageSize.y) * TAN_HALF_FOV, 1.f).normalized().dot(clm::vec3(0.f, 0.f, 1.f));
                    const float vignette = (rayDirection.dot(forward) - smallestDot) * (1.f / ((biggestDot - smallestDot) * vignetteStrength));

                    const uint32_t pixelIndex = (static_cast<uint32_t>(pixelY) * imageSize.x + pixelX) * 3;

                    uint32_t hitModelIndex = INVALID_INDEX;
                    uint32_t hitMeshIndex = INVALID_INDEX;
                    clm::vec3 normal;
                    clm::vec3 intersectionPoint;
                    castRay(models, cameraPos, rayDirection, &hitModelIndex, &hitMeshIndex, &normal, &intersectionPoint);

                    if (hitMeshIndex != INVALID_INDEX)
                    {
                        Material &material = models[hitModelIndex].materials[models[hitModelIndex].meshes[hitMeshIndex].materialIndex];

                        uint32_t shadowHitModelIndex = INVALID_INDEX;
                        castRay(models, intersectionPoint, surfaceToSunDir, &shadowHitModelIndex, nullptr, nullptr, nullptr);
                        if(shadowHitModelIndex != INVALID_INDEX)
                        {
                            pixelColor = material.tinted({0.f, 0.f, 0.f, 255.f}, 0.5f).color;
                        }
                        else
                        {
                            pixelColor = material.color;
                        }
                    }

                    image[pixelIndex] = static_cast<uint8_t>(pixelColor.x * vignette);
                    image[pixelIndex + 1] = static_cast<uint8_t>(pixelColor.y * vignette);
                    image[pixelIndex + 2] = static_cast<uint8_t>(pixelColor.z * vignette);
                }
            }
        };

        for (unsigned int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
            workers.emplace_back(renderRows, threadIndex);

        for (std::thread &worker : workers)
            worker.join();

        return image;
    }
}