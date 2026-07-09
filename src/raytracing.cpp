// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

#include <thread>

namespace CL
{
    [[nodiscard]] bool RayIntersectsTriangle(const clm::vec3 &rayOrigin, const clm::vec3 &rayVector, const std::array<clm::vec3, 3> &pts, clm::vec3 &outIntersectionPoint)
    {
        constexpr float EPSILON = 0.0000001f;

        clm::vec3 edge01 = pts[1] - pts[0];
        clm::vec3 edge02 = pts[2] - pts[0];

        clm::vec3 rayCrossEdge02 = rayVector.cross(edge02);

        float determinant = edge01.dot(rayCrossEdge02);

        if (determinant > -EPSILON && determinant < EPSILON)
            return false;

        float inverseDeterminant = 1 / determinant;
        clm::vec3 vertex0ToRayOrigin = rayOrigin - pts[0];
        float barycentricU = inverseDeterminant * vertex0ToRayOrigin.dot(rayCrossEdge02);

        if (barycentricU < 0.0 || barycentricU > 1.0)
            return false;

        clm::vec3 originCrossEdge01 = vertex0ToRayOrigin.cross(edge01);

        float barycentricV = inverseDeterminant * rayVector.dot(originCrossEdge01);

        if (barycentricV < 0.0 || barycentricU + barycentricV > 1.0)
            return false;

        float rayDistance = inverseDeterminant * edge02.dot(originCrossEdge01);

        if (rayDistance > EPSILON)
        {
            outIntersectionPoint = rayOrigin + (rayVector.normalized() * (rayDistance * rayVector.length()));
            return true;
        }
        else
        {
            return false;
        }
    }

    void Renderer::castRay(const std::vector<Model> models, const clm::vec3 &rayOrigin, const clm::vec3 &rayDirection, uint32_t *outModelIndex, uint32_t *outMeshIndex, clm::vec3 *outNormal, clm::vec3 *outIntersectionPoint)
    {
        float closestDistance = std::numeric_limits<float>::max();

        for (uint32_t i = 0; i < models.size(); i++)
        {
            clm::mat4 modelMat = models[i].transform.mat();
            for (uint32_t j = 0; j < models[i].meshes.size(); j++)
            {
                const Mesh &mesh = models[i].meshes[j];
                for (size_t indexOffset = 0; indexOffset + 2 < mesh.indices.size(); indexOffset += 3)
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

    const std::vector<uint8_t> Renderer::getImageRayTraced(uint32_t width, uint32_t height)
    {
        std::vector<uint8_t> image(width * height * 3);

        const float tanHalfFov = std::tan(FOV * 0.5f);
        const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

        const unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
        std::vector<std::thread> workers;
        workers.reserve(threadCount);

        const clm::vec3 forward = {cosf(cameraRot.x) * sinf(cameraRot.y), -sinf(cameraRot.x), cosf(cameraRot.x) * cosf(cameraRot.y)};
        const clm::vec3 right = {cosf(cameraRot.y), 0.f, -sinf(cameraRot.y)};
        const clm::vec3 up = {sinf(cameraRot.x) * sinf(cameraRot.y), cosf(cameraRot.x), sinf(cameraRot.x) * cosf(cameraRot.y)};

        auto renderRows = [&](unsigned int threadIndex)
        {
            for (uint32_t pixelY = threadIndex; pixelY < height; pixelY += threadCount)
            {
                for (uint32_t pixelX = 0; pixelX < width; pixelX++)
                {
                    const float ndcX = clm::unitToSignedRange((pixelX + 0.5f) / width) * aspectRatio * tanHalfFov;
                    const float ndcY = -clm::unitToSignedRange((pixelY + 0.5f) / height) * tanHalfFov;

                    const clm::vec3 rayDirection = (right * ndcX + up * ndcY + forward).normalized();

                    clm::vec4 pixelColor = {clearColor.x, clearColor.y, clearColor.z, 1.f};

                    const float smallestDot = clm::vec3(clm::unitToSignedRange(0.5f / width) * aspectRatio * tanHalfFov, -clm::unitToSignedRange(0.5f / height) * tanHalfFov, 1.f).normalized().dot(clm::vec3(0.f, 0.f, 1.f));
                    const float biggestDot = clm::vec3(clm::unitToSignedRange((width / 2.f + 0.5f) / width) * aspectRatio * tanHalfFov, -clm::unitToSignedRange((height / 2.f + 0.5f) / height) * tanHalfFov, 1.f).normalized().dot(clm::vec3(0.f, 0.f, 1.f));
                    const float vignette = (rayDirection.dot(forward) - smallestDot) * (1.f / ((biggestDot - smallestDot) * vignetteStrength));

                    const size_t pixelIndex = (static_cast<size_t>(pixelY) * width + pixelX) * 3;

                    uint32_t hitModelIndex = INVALID_INDEX;
                    uint32_t hitMeshIndex = INVALID_INDEX;
                    clm::vec3 normal;
                    castRay(models, cameraPos, rayDirection, &hitModelIndex, &hitMeshIndex, &normal, nullptr);

                    if (hitMeshIndex != INVALID_INDEX)
                    {
                        float shade = std::max(0.f, normal.dot(surfaceToSunDir));

                        Material &material = models[hitModelIndex].materials[models[hitModelIndex].meshes[hitMeshIndex].materialIndex];

                        pixelColor = material.color * shade;
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