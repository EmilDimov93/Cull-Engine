// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

#include <fstream>
#include <algorithm>
#include <array>

namespace CL
{
    void fillTriangle(std::vector<unsigned char> &image, uint32_t width, uint32_t height, std::array<clm::vec3, 3> pts, Material material)
    {
        int minX = std::max(0.f, std::min({pts[0].x, pts[1].x, pts[2].x}));
        int maxX = std::min(static_cast<float>(width - 1), std::max({pts[0].x, pts[1].x, pts[2].x}));
        int minY = std::max(0.f, std::min({pts[0].y, pts[1].y, pts[2].y}));
        int maxY = std::min(static_cast<float>(height - 1), std::max({pts[0].y, pts[1].y, pts[2].y}));

        auto edgeFunction = [](int x1, int y1, int x2, int y2, int px, int py) -> long long
        {
            return (long long)(x2 - x1) * (py - y1) - (long long)(y2 - y1) * (px - x1);
        };

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
                    image[width * y * 3 + x * 3] = material.color.x;
                    image[width * y * 3 + x * 3 + 1] = material.color.y;
                    image[width * y * 3 + x * 3 + 2] = material.color.z;
                }
            }
        }
    }

    const std::vector<unsigned char> Renderer::getImage()
    {
        std::vector<unsigned char> image(100 * 100 * 3);

        for (uint32_t i = 0; i < 100 * 100 * 3; i += 3)
        {
            image[i] = 0;
            image[i + 1] = 0;
            image[i + 2] = 0;
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
                    points[currPoint] = mesh.vertices[index].pos;

                    if (currPoint == 2)
                    {
                        fillTriangle(image, width, height, points, material);
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

    void Renderer::uploadImage(std::vector<unsigned char> image, std::string filePath)
    {
        std::ofstream file(filePath, std::ios::binary);

        file << "P6\n" << width << ' ' << height << "\n255\n";

        file.write(reinterpret_cast<const char *>(image.data()), static_cast<std::streamsize>(width) * height * 3);
    }

    void Renderer::renderModelsToImage(std::string filePath)
    {
        std::vector<unsigned char> image = getImage();

        uploadImage(image, filePath);
    }

    void Renderer::addModel(Model &model)
    {
        models.push_back(model);
    }
}