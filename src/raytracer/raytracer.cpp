// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "raytracer.hpp"

#include <thread>
#include <fstream>
#include <array>

#include "bvh.hpp"

namespace CL
{
    [[nodiscard]] bool RayIntersectsTriangle(const clm::vec3 &rayOrigin, const clm::vec3 &rayVector, const std::array<Vertex, 3> &vertices, clm::vec3 &outIntersectionPoint, bool &outIsFrontFace)
    {
        constexpr float EPSILON = 1e-7f;
        constexpr float RAY_MIN_DISTANCE = 1e-3f;

        const clm::vec3 edge01 = vertices[1].pos - vertices[0].pos;
        const clm::vec3 edge02 = vertices[2].pos - vertices[0].pos;

        const clm::vec3 rayCrossEdge02 = rayVector.cross(edge02);

        const float determinant = edge01.dot(rayCrossEdge02);

        if (std::abs(determinant) < EPSILON)
            return false;

        outIsFrontFace = determinant > 0.0f;

        const float inverseDeterminant = 1 / determinant;
        const clm::vec3 vertex0ToRayOrigin = rayOrigin - vertices[0].pos;
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

    void castRayBVH(const BVH &bvh, const clm::vec3 &rayOrigin, const clm::vec3 &rayDirection, bool *outHasHit, uint32_t *outMaterialIndex, clm::vec3 *outNormal, clm::uvec2 *outUV, clm::vec3 *outIntersectionPoint)
    {
        std::vector<uint32_t> hitLeaves = traverseBVH(bvh.nodes, rayOrigin, clm::vec3(1 / rayDirection.x, 1 / rayDirection.y, 1 / rayDirection.z));

        uint32_t pickedTriangle = INVALID_INDEX;
        uint32_t pickedLeave = INVALID_INDEX;
        clm::vec3 pickedIntersectionPoint;
        float closestDistance = std::numeric_limits<float>::max();

        for (uint32_t leave : hitLeaves)
        {
            for (uint32_t i = 0; i < bvh.nodes[leave].triangles.size(); i++)
            {
                clm::vec3 intersectionPoint;
                bool isFrontFace;

                if (RayIntersectsTriangle(rayOrigin, rayDirection, bvh.nodes[leave].triangles[i].vertices, intersectionPoint, isFrontFace))
                {
                    const float distance = (intersectionPoint - rayOrigin).length();
                    if (distance < closestDistance)
                    {
                        pickedTriangle = i;
                        pickedLeave = leave;
                        pickedIntersectionPoint = intersectionPoint;
                        closestDistance = distance;
                    }
                }
            }
        }

        if (pickedTriangle != INVALID_INDEX)
        {
            const std::array<Vertex, 3> &vertices = bvh.nodes[pickedLeave].triangles[pickedTriangle].vertices;

            if (outHasHit)
                *outHasHit = true;

            if (outMaterialIndex)
                *outMaterialIndex = bvh.nodes[pickedLeave].triangles[pickedTriangle].material;

            if (outNormal || outUV)
            {
                clm::vec3 barycentric = computeBarycentric(vertices, pickedIntersectionPoint);

                if (outNormal)
                {
                    clm::vec3 smoothNormal = vertices[0].normal * barycentric.x + vertices[1].normal * barycentric.y + vertices[2].normal * barycentric.z;
                    *outNormal = smoothNormal.normalized();
                }

                if (outUV)
                {
                    clm::uvec2 texelCoord =
                        vertices[0].tex * barycentric.x +
                        vertices[1].tex * barycentric.y +
                        vertices[2].tex * barycentric.z;

                    const Material &material = bvh.materials[bvh.nodes[pickedLeave].triangles[pickedTriangle].material];

                    *outUV = clm::uvec2(clm::clamp(texelCoord.x + 0.5f, 0, material.textureSize.x - 1), clm::clamp(texelCoord.y + 0.5f, 0, material.textureSize.y - 1));
                }
            }

            if (outIntersectionPoint)
                *outIntersectionPoint = pickedIntersectionPoint;
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
                    const Vertex &v0 = mesh.vertices[mesh.indices[indexOffset + 0]];
                    const Vertex &v1 = mesh.vertices[mesh.indices[indexOffset + 1]];
                    const Vertex &v2 = mesh.vertices[mesh.indices[indexOffset + 2]];
                    const std::array<Vertex, 3> vertices = {
                        Vertex((modelMat * clm::vec4(v0.pos, 1.f)).xyz(), modelMat * v0.normal),
                        Vertex((modelMat * clm::vec4(v1.pos, 1.f)).xyz(), modelMat * v1.normal),
                        Vertex((modelMat * clm::vec4(v2.pos, 1.f)).xyz(), modelMat * v2.normal)};

                    clm::vec3 intersectionPoint;
                    bool isFrontFace;
                    if (RayIntersectsTriangle(rayOrigin, rayDirection, vertices, intersectionPoint, isFrontFace))
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
                                clm::vec3 edge1 = vertices[1].pos - vertices[0].pos;
                                clm::vec3 edge2 = vertices[2].pos - vertices[0].pos;
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

    void applyPostEffects(ColorAttachment &colorAtt, float vignetteStrength)
    {
        for (uint32_t y = 0; y < colorAtt.size.y; y++)
        {
            for (uint32_t x = 0; x < colorAtt.size.x; x++)
            {
                const float vignetteX = static_cast<float>(x > colorAtt.size.x / 2 ? colorAtt.size.x - x : x) / (colorAtt.size.x / 2);
                const float vignetteY = static_cast<float>(y > colorAtt.size.y / 2 ? colorAtt.size.y - y : y) / (colorAtt.size.y / 2);
                const float vignette = 1.0f - vignetteStrength * (1.0f - (vignetteX + vignetteY) * 0.5f);

                colorAtt.setPixel(x, y, colorAtt.getPixel(x, y) * vignette);
            }
        }
    }

    ColorAttachment renderSceneRayTraced(clm::uvec2 imageSize, const Scene &scene, float fov, float vignetteStrength, uint32_t bvhDepth)
    {
        ColorAttachment colorAtt(imageSize);

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

                        clm::vec4 pixelColor(scene.clearColor, 255.f);

                        uint32_t materialIndex;
                        clm::vec3 normal;
                        clm::vec3 intersectionPoint;
                        clm::uvec2 uv;
                        bool hasHit = false;

                        clm::vec3 rayOrigin = scene.camera.getPos();
                        const clm::vec3 rayDirection = (basis.right * ndcX + basis.up * ndcY + basis.forward).normalized();
                        clm::vec4 accumulatedColor(0.f, 0.f, 0.f, 0.f);
                        static constexpr uint32_t MAX_RAYS_PER_PIXEL = 20;
                        for (uint32_t i = 0; i < MAX_RAYS_PER_PIXEL; i++)
                        {
                            castRayBVH(bvh, rayOrigin, rayDirection, &hasHit, &materialIndex, &normal, &uv, &intersectionPoint);

                            if (hasHit)
                            {
                                // Find base color
                                const Material &material = bvh.materials[materialIndex];
                                clm::vec4 baseColor = material.color;
                                if (material.texturePixels.size() > 0)
                                {
                                    baseColor = clm::vec4(material.texturePixels[(uv.y * material.textureSize.x + uv.x) * 4.f + 0],
                                                          material.texturePixels[(uv.y * material.textureSize.x + uv.x) * 4.f + 1],
                                                          material.texturePixels[(uv.y * material.textureSize.x + uv.x) * 4.f + 2],
                                                          material.color.w);
                                }

                                // Accumulate color
                                const float accumulatedAlpha01 = accumulatedColor.w / 255.f;
                                if (baseColor.w > 254.f)
                                {
                                    pixelColor = accumulatedColor * accumulatedAlpha01 + baseColor * (1.f - accumulatedAlpha01);
                                    break;
                                }
                                else
                                {
                                    const float materialAlpha01 = baseColor.w / 255.f;

                                    const float totalAlpha = accumulatedAlpha01 + materialAlpha01 * (1.f - accumulatedAlpha01);
                                    accumulatedColor = clm::vec4(clm::vec3(accumulatedColor.xyz() * accumulatedAlpha01 + baseColor.xyz() * materialAlpha01 * (1.0f - accumulatedAlpha01)) / totalAlpha, totalAlpha);

                                    accumulatedColor = clm::clamp(accumulatedColor, 0.f, 255.f);
                                }
                            }
                            else
                            {
                                pixelColor = accumulatedColor * accumulatedColor.w + pixelColor * (1.f - accumulatedColor.w);
                                break;
                            }

                            rayOrigin = intersectionPoint + rayDirection * 1e-6f;
                        }

                        if (hasHit)
                        {
                            // Shade base color
                            hasHit = false;
                            materialIndex = INVALID_INDEX;
                            castRayBVH(bvh, intersectionPoint, scene.surfaceToSunDir, &hasHit, &materialIndex, nullptr, nullptr, nullptr);
                            if (hasHit && bvh.materials[materialIndex].color.w > 0.99f)
                                pixelColor = clm::lerp(pixelColor, {0.f, 0.f, 0.f, 255.f}, 0.5f);
                            else
                                pixelColor = clm::clamp(pixelColor * (scene.ambient + std::max(0.f, normal.dot(scene.surfaceToSunDir))), 0.f, 255.f);
                        }

                        colorAtt.setPixel(pixelX, pixelY, pixelColor);
                    }
                }
            };

            for (unsigned int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
                workers.emplace_back(renderRows, threadIndex);

            for (std::thread &worker : workers)
                worker.join();
        }

        applyPostEffects(colorAtt, vignetteStrength);

        return colorAtt;
    }

    void exportAttachmentPPM(const ColorAttachment &colorAtt, const std::string &filePath, bool shouldOpen)
    {
        std::ofstream file(filePath, std::ios::binary);

        if (!file)
            throw std::runtime_error("Failed to open .ppm file");

        file << "P6\n"
             << colorAtt.size.x << ' ' << colorAtt.size.y << "\n255\n";

        file.write(reinterpret_cast<const char *>(colorAtt.image.data()), static_cast<std::streamsize>(colorAtt.size.x) * static_cast<std::streamsize>(colorAtt.size.y) * 3);

        file.close();

        if (shouldOpen)
            std::system("start build\\result.ppm");
    }
}