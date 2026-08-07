// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "editor.hpp"

namespace CL
{
    static void drawTriangleSolid(ColorAttachment &colorAtt, DepthAttachment &depthAtt, std::array<clm::vec3, 3> pts, Material material, float shade)
    {
        const int32_t minX = std::max(0.f, std::min({pts[0].x, pts[1].x, pts[2].x}));
        const int32_t maxX = std::min(colorAtt.size.x - 1.f, std::max({pts[0].x, pts[1].x, pts[2].x}));
        const int32_t minY = std::max(0.f, std::min({pts[0].y, pts[1].y, pts[2].y}));
        const int32_t maxY = std::min(colorAtt.size.y - 1.f, std::max({pts[0].y, pts[1].y, pts[2].y}));

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

                    if (depth < depthAtt.getPixel(x, y))
                    {
                        depthAtt.setPixel(x, y, depth);
                        colorAtt.setPixel(x, y, material.color * shade);
                    }
                }
            }
        }
    }

    void drawLine(clm::vec3 start, clm::vec3 end, ColorAttachment &colorAtt, DepthAttachment *depthAtt, const Material &material, float shade)
    {
        int32_t currentX = static_cast<int32_t>(std::floor(start.x));
        int32_t currentY = static_cast<int32_t>(std::floor(start.y));
        int32_t endX = static_cast<int32_t>(std::floor(end.x));
        int32_t endY = static_cast<int32_t>(std::floor(end.y));

        const auto isInside = [&](int32_t x, int32_t y)
        {
            return x >= 0 && x < colorAtt.size.x && y >= 0 && y < colorAtt.size.y;
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

                const uint32_t pixelIndex = colorAtt.size.x * currentY + currentX;
                if ((depthAtt == nullptr) || (depth < depthAtt->getPixel(currentX, currentY)))
                {
                    if (depthAtt != nullptr)
                        depthAtt->setPixel(currentX, currentY, depth);
                    colorAtt.setPixel(currentX, currentY, material.color * shade);
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

    static void drawTriangleWireframe(ColorAttachment &colorAtt, DepthAttachment &depthAtt, std::array<clm::vec3, 3> pts, Material material, float shade)
    {
        drawLine(pts[0], pts[1], colorAtt, &depthAtt, material, shade);
        drawLine(pts[1], pts[2], colorAtt, &depthAtt, material, shade);
        drawLine(pts[2], pts[0], colorAtt, &depthAtt, material, shade);
    }

    void Editor::drawMesh(const Mesh &mesh, const Material &material, DepthAttachment &depthAtt, bool isSolid, bool hasShading)
    {
        for (uint32_t i = 2; i < mesh.indices.size(); i += 3)
        {
            std::array<clm::vec3, 3> normals;
            std::array<clm::vec3, 3> verticesClip;

            if (!vertShader.run(mesh.vertices[mesh.indices[i - 0]], verticesClip[0], normals[0]))
                continue;
            if (!vertShader.run(mesh.vertices[mesh.indices[i - 1]], verticesClip[1], normals[1]))
                continue;
            if (!vertShader.run(mesh.vertices[mesh.indices[i - 2]], verticesClip[2], normals[2]))
                continue;

            {
                const float signedAreaTimesTwo =
                    (verticesClip[1].x - verticesClip[0].x) * (verticesClip[2].y - verticesClip[0].y) -
                    (verticesClip[2].x - verticesClip[0].x) * (verticesClip[1].y - verticesClip[0].y);

                if (signedAreaTimesTwo <= 0.0f)
                    continue;
            }

            float shade = 1.f;
            if (hasShading)
            {
                static constexpr float ambientLight = 0.25f;
                const clm::vec3 normal = (normals[0] + normals[1] + normals[2]) / 3.f;
                shade = std::max(0.f, normal.dot(scene.surfaceToSunDir) + ambientLight);
            }

            if (isSolid)
                drawTriangleSolid(colorAttMain, depthAtt, verticesClip, material, shade);
            else
                drawTriangleWireframe(colorAttMain, depthAtt, verticesClip, material, shade);
        }
    }

    void Editor::renderSceneRasterized()
    {
        depthAttMain.clear(std::numeric_limits<float>::infinity());
        depthAttGizmo.clear(std::numeric_limits<float>::infinity());

        for (uint32_t i = 0; i < window.size.x * window.size.y * 3; i += 3)
        {
            colorAttMain.image[i] = static_cast<uint8_t>(scene.clearColor.x);
            colorAttMain.image[i + 1] = static_cast<uint8_t>(scene.clearColor.y);
            colorAttMain.image[i + 2] = static_cast<uint8_t>(scene.clearColor.z);
        }

        vertShader.updateUniformBuffer(scene.camera.viewMat(), projectionMat, window.size);

        for (uint32_t modelIndex = 0; modelIndex < scene.models.size(); modelIndex++)
        {
            const clm::mat4 modelMat = scene.models[modelIndex].transform.mat();

            vertShader.updatePushConstant(modelMat);

            for (const Mesh &mesh : scene.models[modelIndex].meshes)
            {
                const Material &material = scene.models[modelIndex].materials[mesh.materialIndex];
                const Material materialTinted = (modelIndex == selectedModelIndex ? material.tinted(clm::vec4(255.f, 255.f, 0.f, 255.f), 0.2f) : material);
                drawMesh(mesh, materialTinted, depthAttMain, (viewMode == VIEW_MODE_SOLID), true);
            }
        }

        if (selectedModelIndex != INVALID_INDEX)
        {
            const clm::vec3 pos = scene.models[selectedModelIndex].transform.pos;
            const clm::vec3 scale(0.2f, 0.4f, 0.2f);

            for (const Mesh &mesh : gizmoArrow.meshes)
            {
                vertShader.updatePushConstant(Model::Transform(pos, {0.f, 0.f, -clm::PI / 2}, scale).mat());
                drawMesh(mesh, Material({255.f, 0.f, 0.f, 255.f}), depthAttGizmo, true, false);
                vertShader.updatePushConstant(Model::Transform(pos, {0.f, 0.f, 0.f}, scale).mat());
                drawMesh(mesh, Material({0.f, 255.f, 0.f, 255.f}), depthAttGizmo, true, false);
                vertShader.updatePushConstant(Model::Transform(pos, {-clm::PI / 2, 0.f, 0.f}, scale).mat());
                drawMesh(mesh, Material({0.f, 0.f, 255.f, 255.f}), depthAttGizmo, true, false);
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
                            colorAttMain.image[j * window.size.x * 3 + i * 3] = static_cast<uint8_t>(color.x);
                            colorAttMain.image[j * window.size.x * 3 + i * 3 + 1] = static_cast<uint8_t>(color.y);
                            colorAttMain.image[j * window.size.x * 3 + i * 3 + 2] = static_cast<uint8_t>(color.z);
                        }
                    }
                }
            }
        };

        if (originClip.w > 0.f && destClip.w > 0.f)
            drawLine({static_cast<float>(originScreen.x), static_cast<float>(originScreen.y), 1.f}, {static_cast<float>(destScreen.x), static_cast<float>(destScreen.y), 1.f}, colorAttMain, nullptr, Material({255.f, 255.f, 0.f, 255.f}), 1.f);

        drawMarker(originScreen, originClip.w, {255.f, 0.f, 0.f});
        drawMarker(destScreen, destClip.w, (hitModel == INVALID_INDEX ? clm::vec3(255.f, 255.f, 0.f) : clm::vec3(0.f, 255.f, 0.f)));
    }
}