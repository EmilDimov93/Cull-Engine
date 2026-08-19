// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "math/clm.hpp"

#include <vector>
#include <limits>
#include <fstream>
#include <stdexcept>

namespace CL
{
    template <typename T>
    struct Attachment
    {
        std::vector<T> image;
        clm::uvec2 size;
        uint32_t channelCount;

        Attachment(clm::uvec2 size, uint32_t channelCount) : image(size.x * size.y * channelCount), size(size), channelCount(channelCount) {}

        void resize(clm::uvec2 size, T value)
        {
            image.resize(size.x * size.y * channelCount, value);
            this->size = size;
        }

        void clear(T value)
        {
            std::fill(image.begin(), image.end(), value);
        }
    };

    struct ColorAttachment : Attachment<uint8_t>
    {
        explicit ColorAttachment(clm::uvec2 size) : Attachment<uint8_t>(size, 3u) {}

        void setPixel(uint32_t x, uint32_t y, clm::vec4 color)
        {
            if (x >= size.x || y >= image.size() / (size.x * channelCount))
                return;

            image[(y * size.x + x) * channelCount] = static_cast<uint8_t>(clm::clamp(color.x, 0.f, 255.f));
            image[(y * size.x + x) * channelCount + 1] = static_cast<uint8_t>(clm::clamp(color.y, 0.f, 255.f));
            image[(y * size.x + x) * channelCount + 2] = static_cast<uint8_t>(clm::clamp(color.z, 0.f, 255.f));
        }

        clm::vec4 getPixel(uint32_t x, uint32_t y)
        {
            if (x >= size.x || y >= image.size() / (size.x * channelCount))
                return clm::vec4(0.f, 0.f, 0.f, 0.f);

            return clm::vec4(image[(y * size.x + x) * channelCount],
                             image[(y * size.x + x) * channelCount + 1],
                             image[(y * size.x + x) * channelCount + 2], 255.f);
        }
    };

    struct DepthAttachment : Attachment<float>
    {
        explicit DepthAttachment(clm::uvec2 size) : Attachment<float>(size, 1u) {}

        void setPixel(uint32_t x, uint32_t y, float value)
        {
            if (x >= size.x || y >= image.size() / (size.x * channelCount))
                return;

            image[y * size.x + x] = value;
        }

        float getPixel(uint32_t x, uint32_t y)
        {
            if (x >= size.x || y >= image.size() / (size.x * channelCount))
                return std::numeric_limits<float>::infinity();

            return image[y * size.x + x];
        }
    };

    inline void exportAttachmentPPM(const ColorAttachment &colorAtt, const std::string &filePath, bool shouldOpen)
    {
        std::ofstream file(filePath, std::ios::binary);

        if (!file)
            throw std::runtime_error("Failed to open .ppm file");

        file << "P6\n"
             << colorAtt.size.x << ' ' << colorAtt.size.y << "\n255\n";

        file.write(reinterpret_cast<const char *>(colorAtt.image.data()), static_cast<std::streamsize>(colorAtt.size.x) * static_cast<std::streamsize>(colorAtt.size.y) * 3);

        file.close();

        if (shouldOpen)
            std::system((std::string("start ") + filePath).c_str());
    }
}