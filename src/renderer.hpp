// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

/*

Two pipelines
1. Runs live in window, uses rasterization
2. Produces result in .ppm format, uses ray-tracing

Move camera: W, A, S, D
Rotate camera: Arrow keys
Render image to .ppm: R
Select model: left mouse button
Delete model: select it then press Delete

*/

#pragma once

#include "clm.hpp"

#include <vector>
#include <cstdint>
#include <string>
#include <fstream>
#include <array>
#include <algorithm>

#include <GLFW/glfw3.h>

namespace CL
{
    struct Vertex
    {
        clm::vec3 pos;

        constexpr Vertex(clm::vec3 pos) : pos(pos) {}
    };

    struct Material
    {
        clm::vec4 color;

        constexpr Material(clm::vec4 color = {255.f, 255.f, 255.f, 255.f}) : color(color) {}

        constexpr Material tinted(clm::vec4 tint, float tintFactor) const
        {
            tint = tint * tintFactor;
            clm::vec4 baseColor = color * (1.f - tintFactor);
            return Material(baseColor + tint);
        }
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        uint32_t materialIndex;

        Mesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> indices, uint32_t materialIndex) : vertices(vertices), indices(indices), materialIndex(materialIndex) {}
    };

    struct Model
    {
        std::vector<Mesh> meshes;
        std::vector<Material> materials;

        struct Transform
        {
            clm::vec3 pos;
            clm::quaternion rot;
            clm::vec3 scale;

            clm::mat4 mat() const
            {
                return clm::translationMat(pos.x, pos.y, pos.z)
                * rot.mat()
                * clm::scaleMat(scale.x, scale.y, scale.z);
            };

            Transform(clm::vec3 pos = {}, clm::vec3 rot = {}, clm::vec3 scale = {1.f, 1.f, 1.f}) : pos(pos), rot(rot), scale(scale) {}
        } transform;

        Model(const std::vector<Mesh> &meshes, const std::vector<Material> &materials, Transform transform = Transform()) : meshes(meshes), materials(materials), transform(transform) {}
        Model() {}
    };

    class Renderer
    {
    public:
        Renderer(clm::vec2 windowSize = {500.f, 500.f}, clm::vec3 clearColor = {0.f, 0.f, 0.f});
        ~Renderer();

        void addModel(Model &model);
        [[nodiscard]] Model loadOBJ(const std::string &filePath);

        void setVignetteStrength(float vignetteStrength) { this->vignetteStrength = vignetteStrength; }

        void setSunDir(clm::vec3 sunToSurfaceDir)
        {
            surfaceToSunDir = sunToSurfaceDir.normalized();
            surfaceToSunDir = surfaceToSunDir * -1;
        }

        void run();

    private:
        static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

        static constexpr float FOV = clm::PI / 3;
        const float TAN_HALF_FOV = std::tan(FOV / 2.f);

        static constexpr clm::vec4 SELECTED_MODEL_COLOR = clm::vec4(255.f, 255.f, 0.f, 255.f);

        Model gizmoArrow;
        static constexpr Material gizmoArrowXMaterial = Material({255.f, 0.f, 0.f, 255.f});
        static constexpr Material gizmoArrowYMaterial = Material({0.f, 255.f, 0.f, 255.f});
        static constexpr Material gizmoArrowZMaterial = Material({0.f, 0.f, 255.f, 255.f});
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

        bool isCursorHighlighted = true;

        std::vector<Model> models;

        clm::vec2 prevMousePos;

        clm::vec3 clearColor;

        clm::vec3 cameraPos;
        clm::vec3 cameraRot;
        clm::mat4 viewMat;

        clm::vec3 surfaceToSunDir = {0.f, 1.f, 0.f};

        uint32_t selectedModelIndex = INVALID_INDEX;

        float dt;

        float vignetteStrength = 0.f;

        GLFWwindow *window;
        clm::vec2 windowSize;
        float aspectRatio;

        [[nodiscard]] const std::vector<uint8_t> getImageRasterized();
        [[nodiscard]] const std::vector<uint8_t> getImageRayTraced(clm::vec2 imageSize);

        void updateCamera();

        uint32_t findHoveredModel(uint32_t mouseX, uint32_t mouseY);

        static void castRay(const std::vector<Model> &models, const clm::vec3 &rayOrigin, const clm::vec3 &rayVector, uint32_t *outModelIndex, uint32_t *outMeshIndex, clm::vec3 *outNormal, clm::vec3 *outIntersectionPoint);
    };
}