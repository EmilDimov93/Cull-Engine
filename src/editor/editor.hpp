// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

#include "../scene/scene.hpp"

#include "../raytracer/raytracer.hpp"

#include "window.hpp"

#include <vector>
#include <cstdint>
#include <array>
#include <algorithm>

namespace CL
{
    constexpr float FOV = clm::PI / 3;
    const float TAN_HALF_FOV = std::tan(FOV / 2.f);
    constexpr float ZNEAR = 0.001f;
    constexpr float ZFAR = 1000.f;

    class Editor
    {
    public:
        Editor(const Scene &scene, clm::uvec2 windowSize = {500, 500});

        void setVignetteStrength(float vignetteStrength) { this->vignetteStrength = vignetteStrength; }
        void setResultImageSize(clm::uvec2 size) { resultImageSize = size; }

        void run();

    private:
        Scene scene;

        Window window;

        static constexpr clm::vec4 SELECTED_MODEL_TINT = clm::vec4(255.f, 255.f, 0.f, 255.f);

        static constexpr Material gizmoArrowXMaterial = Material({255.f, 0.f, 0.f, 255.f});
        static constexpr Material gizmoArrowYMaterial = Material({0.f, 255.f, 0.f, 255.f});
        static constexpr Material gizmoArrowZMaterial = Material({0.f, 0.f, 255.f, 255.f});

        float dt = 0.f;

        clm::mat4 projectionMat;

        std::vector<uint8_t> colorAttachmentMain;
        std::vector<float> depthAttachmentMain;
        std::vector<float> depthAttachmentGizmo;

        clm::ivec2 prevMousePos;

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

        enum EditorViewMode
        {
            EDITOR_VIEW_WIREFRAME,
            EDITOR_VIEW_SOLID
        } editorViewMode = EDITOR_VIEW_WIREFRAME;

        void renderImageRasterized();

        uint32_t findHoveredModel(clm::ivec2 mousePos);
        void drawMesh(const Mesh &mesh, const Material &material, const clm::mat4 &modelMat, std::vector<float> &depthAttachment, bool isSolid, bool hasShading);
        void debugRay(clm::vec3 origin, clm::vec3 dir);

        // Ray-Tracer
        clm::uvec2 resultImageSize = {100u, 100u};
        float vignetteStrength = 0.f;
        uint32_t bvhLevelCount = 10u;
    };
}