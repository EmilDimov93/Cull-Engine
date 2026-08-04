// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "editor.hpp"

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

                    if (depth < depthBuffer[y * imageSize.x + x])
                    {
                        depthBuffer[y * imageSize.x + x] = depth;
                        placePixel3c(clm::clamp(material.color * shade, 0.f, 255.f), x, y, image, imageSize.x);
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

    void Editor::drawMesh(const Mesh &mesh, const Material &material, const clm::mat4 &modelMat, std::vector<float> &depthAttachment, bool isSolid, bool hasShading)
    {
        uint32_t currPoint = 0;
        std::array<clm::vec3, 3> points;
        std::array<clm::vec3, 3> pointsWorld;
        std::array<clm::vec3, 3> pointsView;
        for (uint32_t index : mesh.indices)
        {
            pointsWorld[currPoint] = modelMat * mesh.vertices[index].pos;
            pointsView[currPoint] = scene.camera.viewMat() * pointsWorld[currPoint];

            if (pointsView[currPoint].z < ZNEAR)
            {
                currPoint = 0;
                continue;
            }

            const clm::vec4 pointClip = projectionMat * clm::vec4(pointsView[currPoint], 1.f);
            const clm::vec2 ndc(pointClip.x / pointClip.w, pointClip.y / pointClip.w);

            points[currPoint] = {clm::signedToUnitRange(ndc.x) * window.size.x,
                                 clm::signedToUnitRange(ndc.y) * window.size.y,
                                 pointsView[currPoint].z};

            if (currPoint == 2)
            {
                float shade = 1.f;
                if (hasShading)
                {
                    const clm::vec3 edge1 = pointsWorld[1] - pointsWorld[0];
                    const clm::vec3 edge2 = pointsWorld[2] - pointsWorld[0];
                    const clm::vec3 worldNormal = edge1.cross(edge2).normalized();

                    const float ambient = 0.25f;
                    shade = std::max(0.f, worldNormal.dot(scene.surfaceToSunDir) + ambient);
                }

                if (isSolid)
                    drawTriangleSolid(colorAttachmentMain, window.size, points, depthAttachment, material, shade);
                else
                    drawTriangleWireframe(colorAttachmentMain, window.size, points, depthAttachment, material, shade);
                currPoint = 0;
            }
            else
            {
                currPoint++;
            }
        }
    }

    void Editor::renderImageRasterized()
    {
        std::fill(depthAttachmentMain.begin(), depthAttachmentMain.end(), std::numeric_limits<float>::infinity());
        std::fill(depthAttachmentGizmo.begin(), depthAttachmentGizmo.end(), std::numeric_limits<float>::infinity());

        for (uint32_t i = 0; i < window.size.x * window.size.y * 3; i += 3)
        {
            colorAttachmentMain[i] = static_cast<uint8_t>(scene.clearColor.x);
            colorAttachmentMain[i + 1] = static_cast<uint8_t>(scene.clearColor.y);
            colorAttachmentMain[i + 2] = static_cast<uint8_t>(scene.clearColor.z);
        }

        for (uint32_t modelIndex = 0; modelIndex < scene.models.size(); modelIndex++)
        {
            const clm::mat4 modelMat = scene.models[modelIndex].transform.mat();
            for (const Mesh &mesh : scene.models[modelIndex].meshes)
            {
                const Material &material = scene.models[modelIndex].materials[mesh.materialIndex];
                const Material materialTinted = (modelIndex == selectedModelIndex ? material.tinted(clm::vec4(255.f, 255.f, 0.f, 255.f), 0.2f) : material);
                drawMesh(mesh, materialTinted, modelMat, depthAttachmentMain, (editorViewMode == EDITOR_VIEW_SOLID), true);
            }
        }

        if (selectedModelIndex != INVALID_INDEX)
        {
            const clm::mat4 gizmoArrowXModelMat = Model::Transform(scene.models[selectedModelIndex].transform.pos, {0.f, 0.f, -clm::PI / 2}, {0.2f, 0.4f, 0.2f}).mat();
            const clm::mat4 gizmoArrowYModelMat = Model::Transform(scene.models[selectedModelIndex].transform.pos, {0.f, 0.f, 0.f}, {0.2f, 0.4f, 0.2f}).mat();
            const clm::mat4 gizmoArrowZModelMat = Model::Transform(scene.models[selectedModelIndex].transform.pos, {-clm::PI / 2, 0.f, 0.f}, {0.2f, 0.4f, 0.2f}).mat();

            for (const Mesh &mesh : gizmoArrow.meshes)
            {
                drawMesh(mesh, Material({255.f, 0.f, 0.f, 255.f}), gizmoArrowXModelMat, depthAttachmentGizmo, true, false);
                drawMesh(mesh, Material({0.f, 255.f, 0.f, 255.f}), gizmoArrowYModelMat, depthAttachmentGizmo, true, false);
                drawMesh(mesh, Material({0.f, 0.f, 255.f, 255.f}), gizmoArrowZModelMat, depthAttachmentGizmo, true, false);
            }
        }
    }

    void Editor::debugRay(clm::vec3 origin, clm::vec3 dir)
    {
        const clm::vec4 originClip = projectionMat * scene.camera.viewMat() * clm::vec4(origin, 1.f);
        const clm::vec3 originNdc(originClip.x / originClip.w, originClip.y / originClip.w, originClip.z / originClip.w);
        const clm::ivec2 originScreen(static_cast<int32_t>(clm::signedToUnitRange(originNdc.x) * window.size.x), static_cast<int32_t>(clm::signedToUnitRange(originNdc.y) * window.size.y));

        uint32_t hitModel = INVALID_INDEX;
        clm::vec3 intersectionPoint;
        castRay(scene.models, origin, dir, &hitModel, nullptr, nullptr, &intersectionPoint);

        if (hitModel == INVALID_INDEX)
            intersectionPoint = origin + dir * 10.f;

        const clm::vec4 destClip = projectionMat * scene.camera.viewMat() * clm::vec4(intersectionPoint, 1.f);
        const clm::vec3 destNdc(destClip.x / destClip.w, destClip.y / destClip.w, destClip.z / destClip.w);
        const clm::ivec2 destScreen(static_cast<int32_t>(clm::signedToUnitRange(destNdc.x) * window.size.x), static_cast<int32_t>(clm::signedToUnitRange(destNdc.y) * window.size.y));

        auto drawMarker = [&](clm::ivec2 screen, float w, clm::vec3 color)
        {
            static constexpr uint32_t markerSize = 10;
            if (w > 0.f)
            {
                if (screen.x >= markerSize && screen.x < window.size.x - markerSize && screen.y >= markerSize && screen.y < window.size.y - markerSize)
                {
                    for (uint32_t i = screen.x - markerSize; i < screen.x + markerSize; i++)
                    {
                        for (uint32_t j = screen.y - markerSize; j < screen.y + markerSize; j++)
                        {
                            colorAttachmentMain[j * window.size.x * 3 + i * 3] = static_cast<uint8_t>(color.x);
                            colorAttachmentMain[j * window.size.x * 3 + i * 3 + 1] = static_cast<uint8_t>(color.y);
                            colorAttachmentMain[j * window.size.x * 3 + i * 3 + 2] = static_cast<uint8_t>(color.z);
                        }
                    }
                }
            }
        };

        if (originClip.w > 0.f && destClip.w > 0.f)
            drawLine({static_cast<float>(originScreen.x), static_cast<float>(originScreen.y), 1.f}, {static_cast<float>(destScreen.x), static_cast<float>(destScreen.y), 1.f}, colorAttachmentMain.data(), nullptr, window.size, Material({255.f, 255.f, 0.f, 255.f}), 1.f);

        drawMarker(originScreen, originClip.w, {255.f, 0.f, 0.f});
        drawMarker(destScreen, destClip.w, (hitModel == INVALID_INDEX ? clm::vec3(255.f, 255.f, 0.f) : clm::vec3(0.f, 255.f, 0.f)));
    }
}