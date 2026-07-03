// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

namespace CL
{
    void Renderer::renderModelsToImage(std::string filePath, uint32_t width, uint32_t height, RenderMode renderMode)
    {
        std::vector<uint8_t> image;
        switch (renderMode)
        {
        case RENDER_MODE_RASTERIZATION:
            image = getImageRasterized(width, height);
            break;
        case RENDER_MODE_RAY_TRACING:
            image = getImageRayTraced(width, height);
            break;
        }

        std::ofstream file(filePath, std::ios::binary);

        file << "P6\n"
             << width << ' ' << height << "\n255\n";

        file.write(reinterpret_cast<const char *>(image.data()), static_cast<std::streamsize>(width) * height * 3);
    }

    void Renderer::addModel(Model &model)
    {
        models.push_back(model);
    }
}