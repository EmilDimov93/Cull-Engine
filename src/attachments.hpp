// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "math/clm.hpp"

#include <vector>

namespace CL
{

    template <typename T>
    struct Attachment
    {
    public:
        std::vector<T> image;
        clm::uvec2 size;

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

        // place pixel

    private:
        uint32_t channelCount;
    };

    struct ColorAttachment : Attachment<uint8_t>
    {
        explicit ColorAttachment(clm::uvec2 size) : Attachment<uint8_t>(size, 3u) {}
    };

    struct DepthAttachment : Attachment<float>
    {
        explicit DepthAttachment(clm::uvec2 size) : Attachment<float>(size, 1u) {}
    };
}