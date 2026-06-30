// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

#include <fstream>

namespace CL
{
    void Renderer::renderModelsToImage(const char *filePath, int width, int height)
    {
        char rgbData[100 * 100 * 3];
        for (uint32_t i = 0; i < 100 * 100 * 3; i += 3)
        {
            rgbData[i] = 255;
            rgbData[i + 1] = 0;
            rgbData[i + 2] = 0;
        }

        std::ofstream file(filePath, std::ios::binary);
        file << "P6\n" << width << ' ' << height << "\n255\n";
        file.write(rgbData, static_cast<std::streamsize>(width) * height * 3);
    }

    void Renderer::addModel(Model &model)
    {
        models.push_back(model);
    }
}