// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"
#include <iostream>
namespace CL
{
    void fillTriangle(std::vector<uint8_t> &image, uint32_t width, uint32_t height, std::array<clm::vec3, 3> pts, std::vector<float> &depthBuffer, Material material, float shade)
    {
        int minX = std::max(0.f, std::min({pts[0].x, pts[1].x, pts[2].x}));
        int maxX = std::min(static_cast<float>(width - 1), std::max({pts[0].x, pts[1].x, pts[2].x}));
        int minY = std::max(0.f, std::min({pts[0].y, pts[1].y, pts[2].y}));
        int maxY = std::min(static_cast<float>(height - 1), std::max({pts[0].y, pts[1].y, pts[2].y}));

        auto edgeFunction = [](int x1, int y1, int x2, int y2, int px, int py) -> long long
        {
            return (long long)(x2 - x1) * (py - y1) - (long long)(y2 - y1) * (px - x1);
        };

        long long area = edgeFunction(pts[0].x, pts[0].y, pts[1].x, pts[1].y, pts[2].x, pts[2].y);
        if (area == 0)
            return;

        for (int x = minX; x <= maxX; ++x)
        {
            for (int y = minY; y <= maxY; ++y)
            {
                long long w0 = edgeFunction(pts[1].x, pts[1].y, pts[2].x, pts[2].y, x, y);
                long long w1 = edgeFunction(pts[2].x, pts[2].y, pts[0].x, pts[0].y, x, y);
                long long w2 = edgeFunction(pts[0].x, pts[0].y, pts[1].x, pts[1].y, x, y);

                bool hasNeg = (w0 < 0) || (w1 < 0) || (w2 < 0);
                bool hasPos = (w0 > 0) || (w1 > 0) || (w2 > 0);

                if (!(hasNeg && hasPos))
                {
                    float b0 = static_cast<float>(w0) / area;
                    float b1 = static_cast<float>(w1) / area;
                    float b2 = static_cast<float>(w2) / area;
                    float depth = b0 * pts[0].z + b1 * pts[1].z + b2 * pts[2].z;

                    uint32_t pixelIndex = width * y + x;
                    if (depth < depthBuffer[pixelIndex])
                    {
                        depthBuffer[pixelIndex] = depth;
                        image[pixelIndex * 3] = static_cast<uint8_t>(material.color.x * shade);
                        image[pixelIndex * 3 + 1] = static_cast<uint8_t>(material.color.y * shade);
                        image[pixelIndex * 3 + 2] = static_cast<uint8_t>(material.color.z * shade);
                    }
                }
            }
        }
    }

    const std::vector<uint8_t> Renderer::getImageRasterized()
    {
        const float nearPlane = 0.001f;

        std::vector<uint8_t> image(windowWidth * windowHeight * 3);
        std::vector<float> depthBuffer(windowWidth * windowHeight, std::numeric_limits<float>::infinity());

        for (uint32_t i = 0; i < windowWidth * windowHeight * 3; i += 3)
        {
            image[i] = clearColor.x;
            image[i + 1] = clearColor.y;
            image[i + 2] = clearColor.z;
        }

        for (uint32_t modelIndex = 0; modelIndex < models.size(); modelIndex++)
        {
            clm::mat4 modelMat = models[modelIndex].transform.mat();
            for (const Mesh &mesh : models[modelIndex].meshes)
            {
                const Material &material = models[modelIndex].materials[mesh.materialIndex];
                uint32_t currPoint = 0;
                std::array<clm::vec3, 3> points;
                std::array<clm::vec3, 3> pointsWorld;
                std::array<clm::vec3, 3> pointsView;
                for (uint32_t index : mesh.indices)
                {
                    pointsWorld[currPoint] = modelMat * mesh.vertices[index].pos;
                    pointsView[currPoint] = viewMat * pointsWorld[currPoint];
                    const float tanHalfFov = std::tan(FOV * 0.5f);

                    if (pointsView[currPoint].z < nearPlane)
                    {
                        currPoint = 0;
                        continue;
                    }

                    const float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
                    const float ndcX = pointsView[currPoint].x / (pointsView[currPoint].z * tanHalfFov * aspectRatio);
                    const float ndcY = pointsView[currPoint].y / (pointsView[currPoint].z * tanHalfFov);

                    points[currPoint] = {clm::signedToUnitRange(ndcX) * windowWidth,
                                         clm::signedToUnitRange(ndcY) * windowHeight,
                                         pointsView[currPoint].z};

                    if (currPoint == 2)
                    {
                        clm::vec3 edge1 = pointsWorld[1] - pointsWorld[0];
                        clm::vec3 edge2 = pointsWorld[2] - pointsWorld[0];
                        clm::vec3 worldNormal = edge1.cross(edge2).normalized();

                        float shade = std::max(0.f, worldNormal.dot(surfaceToSunDir));

                        fillTriangle(image, windowWidth, windowHeight, points, depthBuffer, (modelIndex == selectedModelIndex ? SELECTED_MODEL_MATERIAL : material), shade);
                        currPoint = 0;
                    }
                    else
                    {
                        currPoint++;
                    }
                }
            }
        }

        if (selectedModelIndex != INVALID_INDEX)
        {
            const clm::mat4 arrowXModelMat = Model::Transform(models[selectedModelIndex].transform.pos, {0.f, 0.f, -clm::PI / 2}, {0.2f, 0.4f, 0.2f}).mat();
            const clm::mat4 arrowYModelMat = Model::Transform(models[selectedModelIndex].transform.pos, {0.f, 0.f, 0.f}, {0.2f, 0.4f, 0.2f}).mat();
            const clm::mat4 arrowZModelMat = Model::Transform(models[selectedModelIndex].transform.pos, {-clm::PI / 2, 0.f, 0.f}, {0.2f, 0.4f, 0.2f}).mat();

            std::vector<float> arrowDepthBuffer(windowWidth * windowHeight, std::numeric_limits<float>::infinity());
            auto drawArrow = [&](clm::mat4 modelMat, Material material)
            {
                for (const Mesh &mesh : arrow.meshes)
                {
                    uint32_t currPoint = 0;
                    std::array<clm::vec3, 3> points;
                    std::array<clm::vec3, 3> pointsWorld;
                    std::array<clm::vec3, 3> pointsView;
                    for (uint32_t index : mesh.indices)
                    {
                        pointsWorld[currPoint] = modelMat * mesh.vertices[index].pos;
                        pointsView[currPoint] = viewMat * pointsWorld[currPoint];
                        const float tanHalfFov = std::tan(FOV * 0.5f);

                        if (pointsView[currPoint].z < nearPlane)
                        {
                            currPoint = 0;
                            continue;
                        }

                        const float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
                        const float ndcX = pointsView[currPoint].x / (pointsView[currPoint].z * tanHalfFov * aspectRatio);
                        const float ndcY = pointsView[currPoint].y / (pointsView[currPoint].z * tanHalfFov);

                        points[currPoint] = {clm::signedToUnitRange(ndcX) * windowWidth,
                                             clm::signedToUnitRange(ndcY) * windowHeight,
                                             pointsView[currPoint].z};

                        if (currPoint == 2)
                        {
                            fillTriangle(image, windowWidth, windowHeight, points, arrowDepthBuffer, material, 1.f);
                            currPoint = 0;
                        }
                        else
                        {
                            currPoint++;
                        }
                    }
                }
            };

            drawArrow(arrowXModelMat, arrowXMaterial);
            drawArrow(arrowYModelMat, arrowYMaterial);
            drawArrow(arrowZModelMat, arrowZMaterial);
        }

        return image;
    }
}