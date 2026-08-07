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
        void updateUniformBuffer(clm::mat4 viewMat, clm::mat4 projectionMat, clm::uvec2 windowSize)
        {
            this->viewMat = viewMat;
            this->projectionMat = projectionMat;
            this->windowSize = windowSize;
        }

        clm::mat4 modelMat;
        void updatePushConstant(clm::mat4 modelMat) { this->modelMat = modelMat; }

        bool run(Vertex inVertex, clm::vec3 &outVerticesClip, clm::vec3 &outNormal)
        {
            const clm::vec4 world = modelMat * clm::vec4(inVertex.pos, 1.f);
            const clm::vec4 view = viewMat * world;

            if (view.z < zNear)
            {
                return false;
            }

            const clm::vec4 pointClip = projectionMat * view;
            const clm::vec2 ndc(pointClip.x / pointClip.w, pointClip.y / pointClip.w);

            {
                outNormal = (modelMat * inVertex.normal).normalized();
            }

            outVerticesClip = clm::vec3(clm::signedToUnitRange(ndc.x) * windowSize.x,
                             clm::signedToUnitRange(ndc.y) * windowSize.y,
                             view.z);

            return true;
        }
    };
}