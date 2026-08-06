// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

#include "../scene/scene.hpp"

namespace CL
{
    class VertexShader
    {
    public:
        float zNear;
        VertexShader(float zNear) : zNear(zNear) {}

        clm::mat4 viewMat, projectionMat;
        clm::uvec2 windowSize;
        void uniform_buffer(clm::mat4 viewMat, clm::mat4 projectionMat, clm::uvec2 windowSize)
        {
            this->viewMat = viewMat;
            this->projectionMat = projectionMat;
            this->windowSize = windowSize;
        }

        clm::mat4 modelMat;
        void push_constants(clm::mat4 modelMat) { this->modelMat = modelMat; }

        clm::vec3 run(Vertex vertex, clm::mat4 modelMat, clm::vec3 &normal)
        {
            const clm::vec4 world = modelMat * clm::vec4(vertex.pos, 1.f);
            const clm::vec4 view = viewMat * world;

            if (view.z < zNear)
            {
                // invalid
            }

            const clm::vec4 pointClip = projectionMat * view;
            const clm::vec2 ndc(pointClip.x / pointClip.w, pointClip.y / pointClip.w);

            {
                normal = (modelMat * vertex.normal).normalized();
            }

            return clm::vec3(clm::signedToUnitRange(ndc.x) * windowSize.x,
                             clm::signedToUnitRange(ndc.y) * windowSize.y,
                             view.z);
        }
    };
}