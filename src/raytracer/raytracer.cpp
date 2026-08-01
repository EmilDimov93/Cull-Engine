// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "raytracer.hpp"

#include <thread>
#include <fstream>
#include <array>

#include "bvh.hpp"

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

    std::vector<uint32_t> traverseBVH(const std::vector<Node> &nodes, const clm::vec3 &origin, const clm::vec3 &invDir)
    {
        std::vector<uint32_t> hitLeaves;

        uint32_t stack[64];
        int stackPointer = 0;
        stack[stackPointer++] = 0;

        while (stackPointer > 0)
        {
            uint32_t index = stack[--stackPointer];

            if (!nodes[index].volume.intersectsRay(origin, invDir))
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

    clm::vec3 computeBarycentric(const std::array<Vertex, 3> &vertices, clm::vec3 p)
    {
        clm::vec3 edge1 = vertices[1].pos - vertices[0].pos;
        clm::vec3 edge2 = vertices[2].pos - vertices[0].pos;
        clm::vec3 edge3 = p - vertices[0].pos;

        float dot11 = edge1.dot(edge1);
        float dot12 = edge1.dot(edge2);
        float dot22 = edge2.dot(edge2);
        float dot31 = edge3.dot(edge1);
        float dot32 = edge3.dot(edge2);

        float denominator = dot11 * dot22 - dot12 * dot12;

        float weightB = (dot22 * dot31 - dot12 * dot32) / denominator;
        float weightC = (dot11 * dot32 - dot12 * dot31) / denominator;
        float weightA = 1.0f - weightB - weightC;

        return clm::vec3(weightA, weightB, weightC);
    }

    void castRayBVH(const BVH &bvh, const clm::vec3 &rayOrigin, const clm::vec3 &rayDirection, bool *outHasHit, Material *outMaterial, clm::vec3 *outNormal, clm::vec3 *outIntersectionPoint)
    {
        std::vector<uint32_t> hitLeaves = traverseBVH(bvh.nodes, rayOrigin, clm::vec3(1 / rayDirection.x, 1 / rayDirection.y, 1 / rayDirection.z));

        float closestDistance = std::numeric_limits<float>::max();

        for (uint32_t leave : hitLeaves)
        {
            for (uint32_t i = 0; i < bvh.nodes[leave].triangles.size(); i++)
            {
                const std::array<Vertex, 3> &vertices = bvh.nodes[leave].triangles[i].vertices;
                clm::vec3 intersectionPoint;
                if (RayIntersectsTriangle(rayOrigin, rayDirection, {vertices[0].pos, vertices[1].pos, vertices[2].pos}, intersectionPoint))
                {
                    const float distance = (intersectionPoint - rayOrigin).length();
                    if (distance < closestDistance)
                    {
                        closestDistance = distance;

                        if (outHasHit)
                            *outHasHit = true;

                        if (outMaterial)
                            *outMaterial = bvh.materials[bvh.nodes[leave].triangles[i].material];

                        if (outNormal)
                        {
                            clm::vec3 barycentric = computeBarycentric(vertices, intersectionPoint);
                            clm::vec3 smoothNormal = vertices[0].normal * barycentric.x + vertices[1].normal * barycentric.y + vertices[2].normal * barycentric.z;
                            *outNormal = smoothNormal.normalized();
                        }

                        if (outIntersectionPoint)
                            *outIntersectionPoint = intersectionPoint;
                    }
                }
            }
        }
    }

    void castRay(const std::vector<Model> &models, const clm::vec3 &rayOrigin, const clm::vec3 &rayDirection, uint32_t *outModelIndex, uint32_t *outMeshIndex, clm::vec3 *outNormal, clm::vec3 *outIntersectionPoint)
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

                            if (outIntersectionPoint)
                                *outIntersectionPoint = intersectionPoint;
                        }
                    }
                }
            }
        }
    }

    void renderSceneToPPM(clm::uvec2 imageSize, const Scene &scene, float fov, float vignetteStrength, uint32_t bvhDepth)
    {
        std::vector<uint8_t> image(imageSize.x * imageSize.y * 3);

        BVH bvh = constructBVH(scene.models, bvhDepth);

        {
            const unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
            std::vector<std::thread> workers;
            workers.reserve(threadCount);

            const Camera::Basis basis = scene.camera.getBasis();

            const float aspectRatio = static_cast<float>(imageSize.x) / imageSize.y;
            const float tanHalfFov = std::tan(fov / 2.f);

            auto renderRows = [&](unsigned int threadIndex)
            {
                for (uint32_t pixelY = threadIndex; pixelY < imageSize.y; pixelY += threadCount)
                {
                    for (uint32_t pixelX = 0; pixelX < imageSize.x; pixelX++)
                    {
                        const float ndcX = clm::unitToSignedRange((pixelX + 0.5f) / imageSize.x) * aspectRatio * tanHalfFov;
                        const float ndcY = -clm::unitToSignedRange((pixelY + 0.5f) / imageSize.y) * tanHalfFov;

                        const clm::vec3 rayDirection = (basis.right * ndcX + basis.up * ndcY + basis.forward).normalized();

                        clm::vec4 pixelColor(scene.clearColor, 255.f);

                        const float vignetteX = static_cast<float>(pixelX > imageSize.x / 2 ? imageSize.x - pixelX : pixelX) / (imageSize.x / 2);
                        const float vignetteY = static_cast<float>(pixelY > imageSize.y / 2 ? imageSize.y - pixelY : pixelY) / (imageSize.y / 2);
                        const float vignette = 1.0f - vignetteStrength * (1.0f - (vignetteX + vignetteY) * 0.5f);

                        Material material;
                        clm::vec3 normal;
                        clm::vec3 intersectionPoint;
                        bool hasHit = false;

                        clm::vec3 rayOrigin = scene.camera.getPos();
                        clm::vec4 accumulatedColor(0.f, 0.f, 0.f, 0.f);
                        static constexpr uint32_t MAX_RAYS_PER_PIXEL = 20;
                        for (uint32_t i = 0; i < MAX_RAYS_PER_PIXEL; i++)
                        {
                            castRayBVH(bvh, rayOrigin, rayDirection, &hasHit, &material, &normal, &intersectionPoint);

                            if (!hasHit)
                            {
                                pixelColor = accumulatedColor * accumulatedColor.w + pixelColor * (1.f - accumulatedColor.w);
                                break;
                            }
                            else
                            {
                                const float accumulatedAlpha01 = accumulatedColor.w / 255.f;
                                if (material.color.w > 254.f)
                                {
                                    pixelColor = accumulatedColor * accumulatedAlpha01 + material.color * (1.f - accumulatedAlpha01);
                                    break;
                                }
                                else
                                {
                                    const float materialAlpha01 = material.color.w / 255.f;

                                    const float totalAlpha = accumulatedAlpha01 + materialAlpha01 * (1.f - accumulatedAlpha01);
                                    accumulatedColor = clm::vec4(clm::vec3(accumulatedColor.xyz() * accumulatedAlpha01 + material.color.xyz() * materialAlpha01 * (1.0f - accumulatedAlpha01)) / totalAlpha, totalAlpha);

                                    accumulatedColor = clm::clamp(accumulatedColor, 0.f, 255.f);
                                }
                            }

                            rayOrigin = intersectionPoint + rayDirection * 1e-6f;
                        }

                        if (hasHit)
                        {
                            hasHit = false;
                            castRayBVH(bvh, intersectionPoint, scene.surfaceToSunDir, &hasHit, nullptr, nullptr, nullptr);
                            if (hasHit)
                            {
                                pixelColor = material.tinted({0.f, 0.f, 0.f, 255.f}, 0.5f).color;
                            }
                            else
                            {
                                pixelColor = material.tinted({0.f, 0.f, 0.f, 255.f}, 1.f - clm::signedToUnitRange(normal.dot(scene.surfaceToSunDir))).color;
                            }
                        }

                        placePixel3c(pixelColor * vignette, pixelX, pixelY, image, imageSize.x);
                    }
                }
            };

            for (unsigned int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
                workers.emplace_back(renderRows, threadIndex);

            for (std::thread &worker : workers)
                worker.join();
        }

        {
            std::ofstream file("build/result.ppm", std::ios::binary);

            if (!file)
                throw std::runtime_error("Failed to open build/result.ppm");

            file << "P6\n"
                 << imageSize.x << ' ' << imageSize.y << "\n255\n";

            file.write(reinterpret_cast<const char *>(image.data()), static_cast<std::streamsize>(imageSize.x) * static_cast<std::streamsize>(imageSize.y) * 3);

            file.close();

            std::system("start build\\result.ppm");
        }
    }
}