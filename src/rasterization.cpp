// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

namespace CL
{
    void fillTriangle(std::vector<uint8_t> &image, uint32_t width, uint32_t height, std::array<clm::vec3, 3> pts, std::vector<float> &depthBuffer, Material material, float shade)
    {
        const int32_t minX = std::max(0.f, std::min({pts[0].x, pts[1].x, pts[2].x}));
        const int32_t maxX = std::min(static_cast<float>(width - 1), std::max({pts[0].x, pts[1].x, pts[2].x}));
        const int32_t minY = std::max(0.f, std::min({pts[0].y, pts[1].y, pts[2].y}));
        const int32_t maxY = std::min(static_cast<float>(height - 1), std::max({pts[0].y, pts[1].y, pts[2].y}));

        auto edgeFunction = [](int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t px, int32_t py) -> int64_t
        {
            return static_cast<int64_t>(x2 - x1) * (py - y1) - static_cast<int64_t>(y2 - y1) * (px - x1);
        };

        const int64_t area = edgeFunction(pts[0].x, pts[0].y, pts[1].x, pts[1].y, pts[2].x, pts[2].y);
        if (area == 0)
            return;

        for (int32_t x = minX; x <= maxX; ++x)
        {
            for (int32_t y = minY; y <= maxY; ++y)
            {
                const int64_t w0 = edgeFunction(pts[1].x, pts[1].y, pts[2].x, pts[2].y, x, y);
                const int64_t w1 = edgeFunction(pts[2].x, pts[2].y, pts[0].x, pts[0].y, x, y);
                const int64_t w2 = edgeFunction(pts[0].x, pts[0].y, pts[1].x, pts[1].y, x, y);

                const bool hasNeg = (w0 < 0) || (w1 < 0) || (w2 < 0);
                const bool hasPos = (w0 > 0) || (w1 > 0) || (w2 > 0);

                if (!(hasNeg && hasPos))
                {
                    const float b0 = static_cast<float>(w0) / area;
                    const float b1 = static_cast<float>(w1) / area;
                    const float b2 = static_cast<float>(w2) / area;
                    const float depth = b0 * pts[0].z + b1 * pts[1].z + b2 * pts[2].z;

                    const uint32_t pixelIndex = width * y + x;
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
            const clm::mat4 modelMat = models[modelIndex].transform.mat();
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
                        const clm::vec3 edge1 = pointsWorld[1] - pointsWorld[0];
                        const clm::vec3 edge2 = pointsWorld[2] - pointsWorld[0];
                        const clm::vec3 worldNormal = edge1.cross(edge2).normalized();

                        const float shade = std::max(0.f, worldNormal.dot(surfaceToSunDir));

                        fillTriangle(image, windowWidth, windowHeight, points, depthBuffer, (modelIndex == selectedModelIndex ? material.tinted(SELECTED_MODEL_COLOR, 0.2f) : material), shade);
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
            const clm::mat4 gizmoArrowXModelMat = Model::Transform(models[selectedModelIndex].transform.pos, {0.f, 0.f, -clm::PI / 2}, {0.2f, 0.4f, 0.2f}).mat();
            const clm::mat4 gizmoArrowYModelMat = Model::Transform(models[selectedModelIndex].transform.pos, {0.f, 0.f, 0.f}, {0.2f, 0.4f, 0.2f}).mat();
            const clm::mat4 gizmoArrowZModelMat = Model::Transform(models[selectedModelIndex].transform.pos, {-clm::PI / 2, 0.f, 0.f}, {0.2f, 0.4f, 0.2f}).mat();

            std::vector<float> arrowDepthBuffer(windowWidth * windowHeight, std::numeric_limits<float>::infinity());
            auto drawArrow = [&](clm::mat4 modelMat, Material material)
            {
                for (const Mesh &mesh : gizmoArrow.meshes)
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

            drawArrow(gizmoArrowXModelMat, gizmoArrowXMaterial);
            drawArrow(gizmoArrowYModelMat, gizmoArrowYMaterial);
            drawArrow(gizmoArrowZModelMat, gizmoArrowZMaterial);
        }

        return image;
    }
}