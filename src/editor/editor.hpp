// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

#include "vertexShader.hpp"
#include "window.hpp"

#include "../scene/scene.hpp"
#include "../attachments.hpp"

#include "../raytracer/raytracer.hpp"

#include <vector>
#include <cstdint>
#include <array>
#include <algorithm>

namespace CL
{
    constexpr float FOV = clm::PI / 3;
    constexpr float ZNEAR = 0.001f;
    constexpr float ZFAR = 1000.f;

    class Editor
    {
    public:
        Editor(const Scene &scene, clm::uvec2 windowSize = {500, 500});

        void run();

        // Ray-Tracer data
        clm::uvec2 resultImageSize = {100u, 100u};
        float vignetteStrength = 0.f;
        uint32_t bvhDepth = 10u;

    private:
        Scene scene;

        Window window;

        VertexShader vertShader;

        float dt = 0.f;

        clm::mat4 projectionMat;

        ColorAttachment colorAttMain;
        DepthAttachment depthAttMain;
        DepthAttachment depthAttGizmo;

        uint32_t selectedModelIndex = INVALID_INDEX;

        Model gizmoArrow;

        enum GizmoMode
        {
            GIZMO_MODE_TRANSLATE,
            GIZMO_MODE_ROTATE,
            GIZMO_MODE_SCALE
        } gizmoMode = GIZMO_MODE_TRANSLATE;

        enum GizmoDrag
        {
            GIZMO_DRAG_NONE,
            GIZMO_DRAG_X_ARROW,
            GIZMO_DRAG_Y_ARROW,
            GIZMO_DRAG_Z_ARROW
        } gizmoDrag = GIZMO_DRAG_NONE;

        bool wasGPressed = false;
        clm::ivec2 prevMousePos;

        enum ViewMode
        {
            VIEW_MODE_WIREFRAME,
            VIEW_MODE_SOLID
        } viewMode = VIEW_MODE_WIREFRAME;

        void renderSceneRasterized();

        uint32_t findHoveredModel(clm::ivec2 mousePos);
        void drawMesh(const Mesh &mesh, clm::vec4 color, DepthAttachment &depthAtt, bool isSolid, bool hasShading);
    };
}