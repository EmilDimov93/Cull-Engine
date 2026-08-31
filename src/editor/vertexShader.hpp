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
        clm::mat4 viewMat, projectionMat;
        clm::uvec2 windowSize;
        clm::mat4 modelMat;

        VertexShader(float zNear) : zNear(zNear) {}

        void updateUniformBuffer(clm::mat4 viewMat, clm::mat4 projectionMat, clm::uvec2 windowSize)
        {
            this->viewMat = viewMat;
            this->projectionMat = projectionMat;
            this->windowSize = windowSize;
        }

        void updatePushConstant(clm::mat4 modelMat) { this->modelMat = modelMat; }

        bool run(Vertex inVertex, clm::vec3 &outVerticesClip, clm::vec3 &outNormal)
        {
            const clm::vec4 view = viewMat * modelMat * clm::vec4(inVertex.pos, 1.f);

            if (view.z < zNear)
                return false;

            const clm::vec4 clip = projectionMat * view;

            {
                outNormal = (modelMat * inVertex.normal).normalized();
            }

            {
                outVerticesClip = clm::vec3(clm::signedToUnitRange(clip.x / clip.w) * windowSize.x,
                                            clm::signedToUnitRange(clip.y / clip.w) * windowSize.y,
                                            clip.z / clip.w);
            }

            return true;
        }
    };
}