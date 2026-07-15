// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

namespace CL
{
    static void drawTriangleSolid(std::vector<uint8_t> &image, clm::vec2 imageSize, std::array<clm::vec3, 3> pts, std::vector<float> &depthBuffer, Material material, float shade)
    {
        const int32_t minX = std::max(0.f, std::min({pts[0].x, pts[1].x, pts[2].x}));
        const int32_t maxX = std::min(imageSize.x - 1.f, std::max({pts[0].x, pts[1].x, pts[2].x}));
        const int32_t minY = std::max(0.f, std::min({pts[0].y, pts[1].y, pts[2].y}));
        const int32_t maxY = std::min(imageSize.y - 1.f, std::max({pts[0].y, pts[1].y, pts[2].y}));

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

                    const uint32_t pixelIndex = static_cast<uint32_t>(imageSize.x) * y + x;
                    if (depth < depthBuffer[pixelIndex])
                    {
                        depthBuffer[pixelIndex] = depth;
                        image[pixelIndex * 3] = static_cast<uint8_t>(std::min(material.color.x * shade, 255.f));
                        image[pixelIndex * 3 + 1] = static_cast<uint8_t>(std::min(material.color.y * shade, 255.f));
                        image[pixelIndex * 3 + 2] = static_cast<uint8_t>(std::min(material.color.z * shade, 255.f));
                    }
                }
            }
        }
    }

    static void drawTriangleWireframe(std::vector<uint8_t> &image, clm::vec2 imageSize, std::array<clm::vec3, 3> pts, std::vector<float> &depthBuffer, Material material, float shade)
    {
        auto drawLine = [&](clm::vec3 start, clm::vec3 end)
        {
            const float maxX = imageSize.x - 1.f;
            const float maxY = imageSize.y - 1.f;
            if (start.x < 0.f || start.x > maxX || start.y < 0.f || start.y > maxY || end.x < 0.f || end.x > maxX || end.y < 0.f || end.y > maxY)
                return;

            int32_t currentX = static_cast<int32_t>(start.x);
            int32_t currentY = static_cast<int32_t>(start.y);
            const int32_t endX = static_cast<int32_t>(end.x);
            const int32_t endY = static_cast<int32_t>(end.y);

            const int32_t deltaX = std::abs(endX - currentX);
            const int32_t deltaY = std::abs(endY - currentY);
            const int32_t stepX = (currentX < endX) ? 1 : -1;
            const int32_t stepY = (currentY < endY) ? 1 : -1;
            int32_t error = deltaX - deltaY;

            const int32_t totalSteps = std::max(deltaX, deltaY);
            int32_t stepIndex = 0;

            while (true)
            {
                if (currentX >= 0 && currentX < static_cast<int32_t>(imageSize.x) &&
                    currentY >= 0 && currentY < static_cast<int32_t>(imageSize.y))
                {
                    const float t = (totalSteps == 0) ? 0.f : static_cast<float>(stepIndex) / totalSteps;
                    const float depth = start.z + (end.z - start.z) * t;

                    const uint32_t pixelIndex = static_cast<uint32_t>(imageSize.x) * currentY + currentX;
                    if (depth < depthBuffer[pixelIndex])
                    {
                        depthBuffer[pixelIndex] = depth;
                        image[pixelIndex * 3] = static_cast<uint8_t>(std::min(material.color.x * shade, 255.f));
                        image[pixelIndex * 3 + 1] = static_cast<uint8_t>(std::min(material.color.y * shade, 255.f));
                        image[pixelIndex * 3 + 2] = static_cast<uint8_t>(std::min(material.color.z * shade, 255.f));
                    }
                }

                if (currentX == endX && currentY == endY)
                    break;

                const int32_t doubledError = 2 * error;
                if (doubledError > -deltaY)
                {
                    error -= deltaY;
                    currentX += stepX;
                }
                if (doubledError < deltaX)
                {
                    error += deltaX;
                    currentY += stepY;
                }
                ++stepIndex;
            }
        };

        drawLine(pts[0], pts[1]);
        drawLine(pts[1], pts[2]);
        drawLine(pts[2], pts[0]);
    }

    const std::vector<uint8_t> Renderer::getImageRasterized()
    {
        const float nearPlane = 0.001f;

        std::vector<uint8_t> image(static_cast<uint32_t>(windowSize.x) * static_cast<uint32_t>(windowSize.y) * 3);
        std::vector<float> depthBuffer(static_cast<uint32_t>(windowSize.x) * static_cast<uint32_t>(windowSize.y), std::numeric_limits<float>::infinity());

        for (uint32_t i = 0; i < static_cast<uint32_t>(windowSize.x) * static_cast<uint32_t>(windowSize.y) * 3; i += 3)
        {
            image[i] = static_cast<uint8_t>(clearColor.x);
            image[i + 1] = static_cast<uint8_t>(clearColor.y);
            image[i + 2] = static_cast<uint8_t>(clearColor.z);
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

                    if (pointsView[currPoint].z < nearPlane)
                    {
                        currPoint = 0;
                        continue;
                    }

                    const float ndcX = pointsView[currPoint].x / (pointsView[currPoint].z * TAN_HALF_FOV * aspectRatio);
                    const float ndcY = pointsView[currPoint].y / (pointsView[currPoint].z * TAN_HALF_FOV);

                    points[currPoint] = {clm::signedToUnitRange(ndcX) * windowSize.x,
                                         clm::signedToUnitRange(ndcY) * windowSize.y,
                                         pointsView[currPoint].z};

                    if (currPoint == 2)
                    {
                        const clm::vec3 edge1 = pointsWorld[1] - pointsWorld[0];
                        const clm::vec3 edge2 = pointsWorld[2] - pointsWorld[0];
                        const clm::vec3 worldNormal = edge1.cross(edge2).normalized();

                        const float ambient = 0.25f;
                        const float shade = std::max(0.f, worldNormal.dot(surfaceToSunDir) + ambient);

                        if (editorViewMode == EDITOR_VIEW_WIREFRAME)
                            drawTriangleWireframe(image, windowSize, points, depthBuffer, (modelIndex == selectedModelIndex ? material.tinted(SELECTED_MODEL_COLOR, 0.2f) : material), shade);
                        else
                            drawTriangleSolid(image, windowSize, points, depthBuffer, (modelIndex == selectedModelIndex ? material.tinted(SELECTED_MODEL_COLOR, 0.2f) : material), shade);
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

            std::vector<float> arrowDepthBuffer(static_cast<uint32_t>(windowSize.x) * static_cast<uint32_t>(windowSize.y), std::numeric_limits<float>::infinity());
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

                        if (pointsView[currPoint].z < nearPlane)
                        {
                            currPoint = 0;
                            continue;
                        }

                        const float ndcX = pointsView[currPoint].x / (pointsView[currPoint].z * TAN_HALF_FOV * aspectRatio);
                        const float ndcY = pointsView[currPoint].y / (pointsView[currPoint].z * TAN_HALF_FOV);

                        points[currPoint] = {clm::signedToUnitRange(ndcX) * windowSize.x,
                                             clm::signedToUnitRange(ndcY) * windowSize.y,
                                             pointsView[currPoint].z};

                        if (currPoint == 2)
                        {
                            drawTriangleSolid(image, windowSize, points, arrowDepthBuffer, material, 1.f);
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