// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "raytracer.hpp"

#include <thread>
#include <array>
#include <random>
#include <cmath>

#include "bvh.hpp"

namespace CL
{
    static constexpr float BIAS_EPSILON = 1e-6f;

    struct HitData
    {
        bool hasHit = false;
        uint32_t materialIndex = INVALID_INDEX;
        clm::vec3 normal;
        clm::uvec2 uv;
        clm::vec3 intersectionPoint;
        bool isFrontFace;

        explicit operator bool() const { return hasHit; }
    };

    [[nodiscard]] bool rayIntersectsTriangle(const clm::vec3 &rayOrigin, const clm::vec3 &rayDir, const std::array<Vertex, 3> &vertices, clm::vec3 &outIntersectionPoint, bool &outIsFrontFace)
    {
        constexpr float EPSILON = 1e-7f;
        constexpr float RAY_MIN_DISTANCE = 1e-3f;

        const clm::vec3 edge01 = vertices[1].pos - vertices[0].pos;
        const clm::vec3 edge02 = vertices[2].pos - vertices[0].pos;

        const clm::vec3 rayCrossEdge02 = rayDir.cross(edge02);

        const float determinant = edge01.dot(rayCrossEdge02);

        if (std::fabsf(determinant) < EPSILON)
            return false;

        outIsFrontFace = determinant > 0.f;

        const float inverseDeterminant = 1.f / determinant;
        const clm::vec3 vertex0ToRayOrigin = rayOrigin - vertices[0].pos;
        const float barycentricU = inverseDeterminant * vertex0ToRayOrigin.dot(rayCrossEdge02);

        if (barycentricU < 0.f || barycentricU > 1.f)
            return false;

        const clm::vec3 originCrossEdge01 = vertex0ToRayOrigin.cross(edge01);

        const float barycentricV = inverseDeterminant * rayDir.dot(originCrossEdge01);

        if (barycentricV < 0.f || barycentricU + barycentricV > 1.f)
            return false;

        const float rayDistance = inverseDeterminant * edge02.dot(originCrossEdge01);

        if (rayDistance > RAY_MIN_DISTANCE)
        {
            outIntersectionPoint = rayOrigin + rayDir * rayDistance;
            return true;
        }
        else
        {
            return false;
        }
    }

    clm::vec3 computeBarycentric(const std::array<Vertex, 3> &vertices, clm::vec3 p)
    {
        const clm::vec3 edge1 = vertices[1].pos - vertices[0].pos;
        const clm::vec3 edge2 = vertices[2].pos - vertices[0].pos;
        const clm::vec3 edge3 = p - vertices[0].pos;

        const float dot11 = edge1.dot(edge1);
        const float dot12 = edge1.dot(edge2);
        const float dot22 = edge2.dot(edge2);
        const float dot31 = edge3.dot(edge1);
        const float dot32 = edge3.dot(edge2);

        const float denominator = dot11 * dot22 - dot12 * dot12;

        const float weightB = (dot22 * dot31 - dot12 * dot32) / denominator;
        const float weightC = (dot11 * dot32 - dot12 * dot31) / denominator;
        const float weightA = 1.0f - weightB - weightC;

        return clm::vec3(weightA, weightB, weightC);
    }

    [[nodiscard]] HitData castRayBVH(const BVH &bvh, const Ray &ray)
    {
        HitData hit;

        const std::vector<uint32_t> hitLeaves = bvh.getHitLeaves(ray.origin, ray.direction);

        uint32_t pickedTriangle = INVALID_INDEX;
        uint32_t pickedLeave = INVALID_INDEX;
        clm::vec3 pickedIntersectionPoint;
        bool pickedTriangleIsFrontFace;
        float closestDistance = std::numeric_limits<float>::max();

        for (const uint32_t leave : hitLeaves)
        {
            for (uint32_t i = 0; i < bvh.nodes[leave].triangles.size(); i++)
            {
                clm::vec3 intersectionPoint;
                bool isFrontFace;
                if (rayIntersectsTriangle(ray.origin, ray.direction, bvh.nodes[leave].triangles[i].vertices, intersectionPoint, isFrontFace))
                {
                    const float distance = (intersectionPoint - ray.origin).length();
                    if (distance < closestDistance)
                    {
                        pickedTriangle = i;
                        pickedLeave = leave;
                        pickedIntersectionPoint = intersectionPoint;
                        closestDistance = distance;
                        pickedTriangleIsFrontFace = isFrontFace;
                    }
                }
            }
        }

        if (pickedTriangle != INVALID_INDEX)
        {
            const std::array<Vertex, 3> &vertices = bvh.nodes[pickedLeave].triangles[pickedTriangle].vertices;

            hit.hasHit = true;

            hit.materialIndex = bvh.nodes[pickedLeave].triangles[pickedTriangle].material;

            const clm::vec3 barycentric = computeBarycentric(vertices, pickedIntersectionPoint);

            const clm::vec3 smoothNormal = vertices[0].normal * barycentric.x + vertices[1].normal * barycentric.y + vertices[2].normal * barycentric.z;
            hit.normal = smoothNormal.normalized();

            const clm::vec2 texelCoord =
                vertices[0].tex * barycentric.x +
                vertices[1].tex * barycentric.y +
                vertices[2].tex * barycentric.z;

            const Material &material = bvh.materials[bvh.nodes[pickedLeave].triangles[pickedTriangle].material];

            hit.uv = clm::uvec2(clm::clamp(texelCoord.x + 0.5f, 0, material.textureSize.x - 1), clm::clamp(texelCoord.y + 0.5f, 0, material.textureSize.y - 1));

            hit.intersectionPoint = pickedIntersectionPoint;

            hit.isFrontFace = pickedTriangleIsFrontFace;
        }

        return hit;
    }

    uint32_t castRay(const std::vector<Model> &models, const Ray &ray)
    {
        uint32_t modelIndex = INVALID_INDEX;

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
                    if (rayIntersectsTriangle(ray.origin, ray.direction, vertices, intersectionPoint, isFrontFace))
                    {
                        const float distance = (intersectionPoint - ray.origin).length();
                        if (distance < closestDistance)
                        {
                            closestDistance = distance;

                            modelIndex = i;
                        }
                    }
                }
            }
        }

        return modelIndex;
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

    bool refractRayDir(const clm::vec3 &preHitDir, const clm::vec3 &normal, float eta, clm::vec3 &refracted)
    {
        const float cosHit = normal.dot(preHitDir);
        const float discriminant = 1.0f - eta * eta * (1.0f - cosHit * cosHit);

        if (discriminant < 0.0f)
            return false;

        refracted = preHitDir * eta - normal * (eta * cosHit + sqrtf(discriminant));
        return true;
    }

    ColorAttachment renderSceneRayTraced(clm::uvec2 imageSize, const Scene &scene, float fov, float vignetteStrength)
    {
        ColorAttachment colorAtt(imageSize);

        BVH bvh(scene.models);

        {
            const unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
            std::vector<std::thread> workers;
            workers.reserve(threadCount);

            RayGenerator rayGen(imageSize, scene.camera, fov);

            auto renderRows = [&](unsigned int threadIndex)
            {
                for (uint32_t pixelY = threadIndex; pixelY < imageSize.y; pixelY += threadCount)
                {
                    for (uint32_t pixelX = 0; pixelX < imageSize.x; pixelX++)
                    {
                        static constexpr uint32_t MAX_RAYS_PER_PIXEL = 5;

                        Ray ray = rayGen.generateRay({pixelX, pixelY});

                        clm::vec4 pixelColor(scene.clearColor, 255.f);

                        HitData hit;

                        auto getBaseColor = [](const Material &material, const clm::uvec2 &uv) -> clm::vec4
                        {
                            clm::vec4 baseColor = material.color;
                            if (material.texturePixels.size() > 0)
                            {
                                baseColor = clm::vec4(material.texturePixels[(uv.y * material.textureSize.x + uv.x) * 4.f + 0],
                                                      material.texturePixels[(uv.y * material.textureSize.x + uv.x) * 4.f + 1],
                                                      material.texturePixels[(uv.y * material.textureSize.x + uv.x) * 4.f + 2],
                                                      material.color.w);
                            }
                            return baseColor;
                        };

                        clm::vec4 accumulatedColor(0.f, 0.f, 0.f, 0.f);
                        for (uint32_t i = 0; i < MAX_RAYS_PER_PIXEL; i++)
                        {
                            hit = castRayBVH(bvh, ray);

                            if (hit)
                            {
                                clm::vec4 baseColor = getBaseColor(bvh.materials[hit.materialIndex], hit.uv);

                                const float accumulatedAlpha01 = accumulatedColor.w;
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

                            static constexpr float AIR_IOR = 1.0003f;
                            float eta = ((hit.isFrontFace) ? (AIR_IOR / bvh.materials[hit.materialIndex].ior) : (bvh.materials[hit.materialIndex].ior / AIR_IOR));
                            clm::vec3 orientedNormal = hit.isFrontFace ? hit.normal : (hit.normal * -1.f);
                            clm::vec3 refractedDir;
                            if (refractRayDir(ray.direction, orientedNormal, eta, refractedDir))
                            {
                                ray.origin = hit.intersectionPoint + orientedNormal * BIAS_EPSILON;
                                ray.direction = refractedDir;
                            }
                            else
                            {
                                break;
                            }
                        }

                        auto shade = [&](const HitData hit) -> clm::vec3
                        {
                            clm::vec3 accumulatedRadiance = clm::vec3(1.f, 1.f, 1.f) * scene.ambient;

                            {
                                HitData shadowHit = castRayBVH(bvh, Ray(hit.intersectionPoint + hit.normal * BIAS_EPSILON, scene.surfaceToSunDir));
                                if (!shadowHit || bvh.materials[shadowHit.materialIndex].color.w < 254.f)
                                    accumulatedRadiance += scene.sunLightColor * scene.sunLightIntensity * std::max(0.f, hit.normal.dot(scene.surfaceToSunDir));
                            }

                            for (const PointLight &light : scene.lights)
                            {
                                clm::vec3 surfaceToLightDir = light.pos - hit.intersectionPoint;
                                const float distance = surfaceToLightDir.length();
                                surfaceToLightDir = surfaceToLightDir.normalized();

                                const float dot = std::max(hit.normal.dot(surfaceToLightDir), 0.0f);
                                if (dot <= 0.0f)
                                    continue;

                                HitData shadowHit = castRayBVH(bvh, Ray(hit.intersectionPoint + hit.normal * BIAS_EPSILON, surfaceToLightDir));

                                if (!shadowHit || bvh.materials[shadowHit.materialIndex].color.w < 254.f)
                                {
                                    const float attenuation = 1.0f / (distance * distance);
                                    accumulatedRadiance += light.color * light.intensity * dot * attenuation;
                                }
                            }

                            return accumulatedRadiance;
                        };

                        if (hit)
                            pixelColor = pixelColor * clm::vec4(shade(hit), 1.f);

                        if (hit && bvh.materials[hit.materialIndex].metallic > 0.f)
                        {
                            clm::vec3 reflectedDir = ray.direction - hit.normal * 2.f * (ray.direction.dot(hit.normal));

                            {
                                // Roughness
                                static thread_local std::mt19937 engine(std::random_device{}());
                                static thread_local std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);

                                clm::vec3 offset(unitDistribution(engine), unitDistribution(engine), unitDistribution(engine));
                                offset = offset.normalized();
                                offset = offset * bvh.materials[hit.materialIndex].roughness;

                                reflectedDir = (reflectedDir + offset).normalized();
                            }

                            HitData reflectedHit = castRayBVH(bvh, Ray(hit.intersectionPoint + hit.normal * BIAS_EPSILON, reflectedDir));

                            if (reflectedHit && bvh.materials[reflectedHit.materialIndex].color.w > 254.f)
                            {
                                clm::vec4 reflectionColor = getBaseColor(bvh.materials[reflectedHit.materialIndex], reflectedHit.uv);
                                pixelColor = clm::lerp(pixelColor, reflectionColor, bvh.materials[hit.materialIndex].metallic);

                                pixelColor = pixelColor * clm::vec4(shade(reflectedHit), 1.f);
                            }
                            else
                                pixelColor = clm::lerp(pixelColor, clm::vec4(scene.clearColor, 1.f), bvh.materials[hit.materialIndex].metallic);
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
}