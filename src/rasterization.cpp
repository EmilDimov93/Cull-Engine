// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

namespace CL
{
    void fillTriangle(std::vector<unsigned char> &image, uint32_t width, uint32_t height, std::array<clm::vec3, 3> pts, std::vector<float> &depthBuffer, Material material)
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
                    float depth = 1.f / (b0 * pts[0].z + b1 * pts[1].z + b2 * pts[2].z);

                    uint32_t pixelIndex = width * y + x;
                    if (depth < depthBuffer[pixelIndex])
                    {
                        depthBuffer[pixelIndex] = depth;
                        image[pixelIndex * 3] = material.color.x;
                        image[pixelIndex * 3 + 1] = material.color.y;
                        image[pixelIndex * 3 + 2] = material.color.z;
                    }
                }
            }
        }
    }

    const std::vector<unsigned char> Renderer::getImageRasterized(uint32_t width, uint32_t height)
    {
        std::vector<unsigned char> image(width * height * 3);
        std::vector<float> depthBuffer(width * height, std::numeric_limits<float>::infinity());

        for (uint32_t i = 0; i < width * height * 3; i += 3)
        {
            image[i] = clearColor.x;
            image[i + 1] = clearColor.y;
            image[i + 2] = clearColor.z;
        }

        for (const Model &model : models)
        {
            for (const Mesh &mesh : model.meshes)
            {
                const Material &material = model.materials[mesh.materialIndex];
                uint32_t currPoint = 0;
                std::array<clm::vec3, 3> points;
                for (uint32_t index : mesh.indices)
                {
                    clm::vec3 world = model.transform * mesh.vertices[index].pos;
                    const float tanHalfFov = std::tan(FOV * 0.5f);
                    const float cameraDepth = world.z;
                    const float ndcX = world.x / (cameraDepth * tanHalfFov);
                    const float ndcY = world.y / (cameraDepth * tanHalfFov);

                    points[currPoint] = {(ndcX * 0.5f + 0.5f) * width,
                                         (0.5f - ndcY * 0.5f) * height,
                                         cameraDepth};

                    if (currPoint == 2)
                    {
                        fillTriangle(image, width, height, points, depthBuffer, material);
                        currPoint = 0;
                    }
                    else
                    {
                        currPoint++;
                    }
                }
            }
        }

        return image;
    }
}