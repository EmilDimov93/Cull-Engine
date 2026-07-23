// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

#include <vector>
#include <cstdint>
#include <string>
#include <fstream>
#include <array>
#include <algorithm>
#include <limits>

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
                return clm::mat4::translation(pos.x, pos.y, pos.z) * rot.mat() * clm::mat4::scale(scale.x, scale.y, scale.z);
            };

            Transform(clm::vec3 pos = {}, clm::vec3 rot = {}, clm::vec3 scale = {1.f, 1.f, 1.f}) : pos(pos), rot(rot), scale(scale) {}
        } transform;

        Model(const std::vector<Mesh> &meshes, const std::vector<Material> &materials, Transform transform = Transform()) : meshes(meshes), materials(materials), transform(transform) {}
        Model() {}
    };

    inline constexpr void placePixel3c(clm::vec4 color, uint32_t x, uint32_t y, std::vector<uint8_t> &image, uint32_t imageWidth)
    {
        if(x >= imageWidth || y >= image.size() / imageWidth)
            return;

        image[y * imageWidth * 3 + x * 3] = static_cast<uint8_t>(color.x);
        image[y * imageWidth * 3 + x * 3 + 1] = static_cast<uint8_t>(color.y);
        image[y * imageWidth * 3 + x * 3 + 2] = static_cast<uint8_t>(color.z);
    }

    class Renderer
    {
    public:
        Renderer(clm::uvec2 windowSize = {500, 500}, clm::vec3 clearColor = {0.f, 0.f, 0.f});
        ~Renderer();

        void addModel(Model &model);
        [[nodiscard]] Model loadOBJ(const std::string &filePath);

        void setVignetteStrength(float vignetteStrength) { this->vignetteStrength = vignetteStrength; }

        void setSunDir(clm::vec3 sunToSurfaceDir)
        {
            surfaceToSunDir = sunToSurfaceDir.normalized();
            surfaceToSunDir = surfaceToSunDir * -1;
        }

        void setResultImageSize(clm::uvec2 size) { resultImageSize = size; }

        void runEditor();
        void renderToPPM(clm::uvec2 imageSize);

    private:
        // Shared
        
        static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

        static constexpr float FOV = clm::PI / 3;
        const float TAN_HALF_FOV = std::tan(FOV / 2.f);

        static constexpr float ZNEAR = 0.001f;
        static constexpr float ZFAR = 1000.f;

        std::vector<Model> models;

        clm::vec3 clearColor;

        class Camera
        {
        public:
            struct Basis
            {
                clm::vec3 forward;
                clm::vec3 right;
                clm::vec3 up;
            };

            void update(GLFWwindow *window, float dt);

            clm::mat4 viewMat() { return mat; };

            Basis getBasis()
            {
                return Basis{
                    .forward = clm::vec3(cosf(rot.x) * sinf(rot.y), -sinf(rot.x), cosf(rot.x) * cosf(rot.y)),
                    .right = clm::vec3(cosf(rot.y), 0.f, -sinf(rot.y)),
                    .up = clm::vec3(sinf(rot.x) * sinf(rot.y), cosf(rot.x), sinf(rot.x) * cosf(rot.y))};
            }

            clm::vec3 getPos() { return pos; }

        private:
            clm::vec3 pos;
            clm::vec3 rot;

            clm::mat4 mat;
        } camera;

        clm::vec3 surfaceToSunDir = {0.f, 1.f, 0.f};

        float dt = 0.f;

        static void castRay(const std::vector<Model> &models, const clm::vec3 &rayOrigin, const clm::vec3 &rayVector, uint32_t *outModelIndex, uint32_t *outMeshIndex, clm::vec3 *outNormal, clm::vec3 *outIntersectionPoint);

        // Editor

        static constexpr clm::vec4 SELECTED_MODEL_TINT = clm::vec4(255.f, 255.f, 0.f, 255.f);

        static constexpr Material gizmoArrowXMaterial = Material({255.f, 0.f, 0.f, 255.f});
        static constexpr Material gizmoArrowYMaterial = Material({0.f, 255.f, 0.f, 255.f});
        static constexpr Material gizmoArrowZMaterial = Material({0.f, 0.f, 255.f, 255.f});

        GLFWwindow *window;
        clm::uvec2 windowSize;
        float aspectRatio;
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

        [[nodiscard]] const std::vector<uint8_t> getImageRayTraced(clm::uvec2 imageSize);
    };
}