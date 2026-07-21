// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

namespace CL
{
    static void drawTriangleSolid(std::vector<uint8_t> &image, clm::uvec2 imageSize, std::array<clm::vec3, 3> pts, std::vector<float> &depthBuffer, Material material, float shade)
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

                    const uint32_t pixelIndex = imageSize.x * y + x;
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

    void drawLine(clm::vec3 start, clm::vec3 end, uint8_t *image, float *depthBuffer, const clm::uvec2 &imageSize, const Material &material, float shade)
    {
        int32_t currentX = static_cast<int32_t>(std::floor(start.x));
        int32_t currentY = static_cast<int32_t>(std::floor(start.y));
        int32_t endX = static_cast<int32_t>(std::floor(end.x));
        int32_t endY = static_cast<int32_t>(std::floor(end.y));

        const auto isInside = [&](int32_t x, int32_t y)
        {
            return x >= 0 && x < imageSize.x && y >= 0 && y < imageSize.y;
        };

        if (!isInside(currentX, currentY) && !isInside(endX, endY))
            return;

        if (!isInside(currentX, currentY))
        {
            std::swap(currentX, endX);
            std::swap(currentY, endY);
            std::swap(start, end);
        }

        const int32_t deltaX = std::abs(endX - currentX);
        const int32_t deltaY = std::abs(endY - currentY);
        const int32_t stepX = (currentX < endX) ? 1 : -1;
        const int32_t stepY = (currentY < endY) ? 1 : -1;
        int32_t error = deltaX - deltaY;

        const int32_t totalSteps = std::max(deltaX, deltaY);
        int32_t stepIndex = 0;

        while (true)
        {
            if (isInside(currentX, currentY))
            {
                const float t = (totalSteps == 0) ? 0.f
                                                  : static_cast<float>(stepIndex) / totalSteps;
                const float depth = start.z + (end.z - start.z) * t;

                const uint32_t pixelIndex = imageSize.x * currentY + currentX;
                if ((depthBuffer == nullptr) || (depth < depthBuffer[pixelIndex]))
                {
                    if (depthBuffer != nullptr)
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
    }

    static void drawTriangleWireframe(std::vector<uint8_t> &image, clm::uvec2 imageSize, std::array<clm::vec3, 3> pts, std::vector<float> &depthBuffer, Material material, float shade)
    {
        drawLine(pts[0], pts[1], image.data(), depthBuffer.data(), imageSize, material, shade);
        drawLine(pts[1], pts[2], image.data(), depthBuffer.data(), imageSize, material, shade);
        drawLine(pts[2], pts[0], image.data(), depthBuffer.data(), imageSize, material, shade);
    }

    void Renderer::renderImageRasterized()
    {
        std::fill(depthAttachmentMain.begin(), depthAttachmentMain.end(), std::numeric_limits<float>::infinity());
        std::fill(depthAttachmentGizmo.begin(), depthAttachmentGizmo.end(), std::numeric_limits<float>::infinity());

        for (uint32_t i = 0; i < windowSize.x * windowSize.y * 3; i += 3)
        {
            colorAttachmentMain[i] = static_cast<uint8_t>(clearColor.x);
            colorAttachmentMain[i + 1] = static_cast<uint8_t>(clearColor.y);
            colorAttachmentMain[i + 2] = static_cast<uint8_t>(clearColor.z);
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

                    if (pointsView[currPoint].z < ZNEAR)
                    {
                        currPoint = 0;
                        continue;
                    }

                    const clm::vec4 pointClip = projectionMat * clm::vec4(pointsView[currPoint], 1.f);
                    const clm::vec2 ndc(pointClip.x / pointClip.w, pointClip.y / pointClip.w);

                    points[currPoint] = {clm::signedToUnitRange(ndc.x) * windowSize.x,
                                         clm::signedToUnitRange(ndc.y) * windowSize.y,
                                         pointsView[currPoint].z};

                    if (currPoint == 2)
                    {
                        const clm::vec3 edge1 = pointsWorld[1] - pointsWorld[0];
                        const clm::vec3 edge2 = pointsWorld[2] - pointsWorld[0];
                        const clm::vec3 worldNormal = edge1.cross(edge2).normalized();

                        const float ambient = 0.25f;
                        const float shade = std::max(0.f, worldNormal.dot(surfaceToSunDir) + ambient);

                        if (editorViewMode == EDITOR_VIEW_WIREFRAME)
                            drawTriangleWireframe(colorAttachmentMain, windowSize, points, depthAttachmentMain, (modelIndex == selectedModelIndex ? material.tinted(SELECTED_MODEL_COLOR, 0.2f) : material), shade);
                        else
                            drawTriangleSolid(colorAttachmentMain, windowSize, points, depthAttachmentMain, (modelIndex == selectedModelIndex ? material.tinted(SELECTED_MODEL_COLOR, 0.2f) : material), shade);
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

                        if (pointsView[currPoint].z < ZNEAR)
                        {
                            currPoint = 0;
                            continue;
                        }

                        const clm::vec4 pointClip = projectionMat * clm::vec4(pointsView[currPoint], 1.f);
                        const clm::vec2 ndc(pointClip.x / pointClip.w, pointClip.y / pointClip.w);

                        points[currPoint] = {clm::signedToUnitRange(ndc.x) * windowSize.x,
                                             clm::signedToUnitRange(ndc.y) * windowSize.y,
                                             pointsView[currPoint].z};

                        if (currPoint == 2)
                        {
                            drawTriangleSolid(colorAttachmentMain, windowSize, points, depthAttachmentGizmo, material, 1.f);
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
    }

    void Renderer::debugRay(clm::vec3 origin, clm::vec3 dir, std::vector<uint8_t> &image)
    {
        const clm::vec4 originClip = projectionMat * viewMat * clm::vec4(origin, 1.f);
        const clm::vec3 originNdc(originClip.x / originClip.w, originClip.y / originClip.w, originClip.z / originClip.w);
        const clm::ivec2 originScreen(static_cast<int32_t>(clm::signedToUnitRange(originNdc.x) * windowSize.x), static_cast<int32_t>(clm::signedToUnitRange(originNdc.y) * windowSize.y));

        uint32_t hitModel = INVALID_INDEX;
        clm::vec3 intersectionPoint;
        castRay(models, origin, dir, &hitModel, nullptr, nullptr, &intersectionPoint);

        if (hitModel == INVALID_INDEX)
            intersectionPoint = origin + dir * 10.f;

        const clm::vec4 destClip = projectionMat * viewMat * clm::vec4(intersectionPoint, 1.f);
        const clm::vec3 destNdc(destClip.x / destClip.w, destClip.y / destClip.w, destClip.z / destClip.w);
        const clm::ivec2 destScreen(static_cast<int32_t>(clm::signedToUnitRange(destNdc.x) * windowSize.x), static_cast<int32_t>(clm::signedToUnitRange(destNdc.y) * windowSize.y));

        auto drawMarker = [&](clm::ivec2 screen, float w, clm::vec3 color)
        {
            static constexpr uint32_t markerSize = 10;
            if (w > 0.f)
            {
                if (screen.x >= markerSize && screen.x < windowSize.x - markerSize && screen.y >= markerSize && screen.y < windowSize.y - markerSize)
                {
                    for (uint32_t i = screen.x - markerSize; i < screen.x + markerSize; i++)
                    {
                        for (uint32_t j = screen.y - markerSize; j < screen.y + markerSize; j++)
                        {
                            image[j * windowSize.x * 3 + i * 3] = color.x;
                            image[j * windowSize.x * 3 + i * 3 + 1] = color.y;
                            image[j * windowSize.x * 3 + i * 3 + 2] = color.z;
                        }
                    }
                }
            }
        };

        if (originClip.w > 0.f && destClip.w > 0.f)
            drawLine({static_cast<float>(originScreen.x), static_cast<float>(originScreen.y), 1.f}, {static_cast<float>(destScreen.x), static_cast<float>(destScreen.y), 1.f}, image.data(), nullptr, windowSize, Material({255.f, 255.f, 0.f}), 1.f);

        drawMarker(originScreen, originClip.w, {255.f, 0.f, 0.f});
        drawMarker(destScreen, destClip.w, (hitModel == INVALID_INDEX ? clm::vec3(255.f, 255.f, 0.f) : clm::vec3(0.f, 255.f, 0.f)));
    }
}