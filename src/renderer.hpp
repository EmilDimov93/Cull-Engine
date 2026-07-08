// Copyright 2025 Emil Dimov
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

        Vertex(clm::vec3 pos) : pos(pos) {}
    };

    struct Material
    {
        clm::vec4 color;

        Material(clm::vec4 color = {255.f, 255.f, 255.f, 255.f}) : color(color) {}

        Material tinted(clm::vec4 tint, float tintFactor) const
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
            clm::vec3 rot;
            clm::vec3 scale;

            clm::mat4 mat() const
            {
                return clm::translationMat(pos.x, pos.y, pos.z) * clm::rotationMat(rot.x, rot.y, rot.z) * clm::scaleMat(scale.x, scale.y, scale.z);
            };

            Transform(clm::vec3 pos = {}, clm::vec3 rot = {}, clm::vec3 scale = {1.f, 1.f, 1.f}) : pos(pos), rot(rot), scale(scale) {}
        } transform;

        Model(const std::vector<Mesh> &meshes, const std::vector<Material> &materials, Transform transform = Transform()) : meshes(meshes), materials(materials), transform(transform) {}
        Model() {}
    };

    class Renderer
    {
    public:
        Renderer(clm::vec3 clearColor = {0.f, 0.f, 0.f});
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

        clm::vec4 SELECTED_MODEL_COLOR = clm::vec4(255.f, 255.f, 0.f, 255.f);

        std::vector<Model> models;

        Model arrow;
        Material arrowXMaterial = Material({255.f, 0.f, 0.f, 255.f});
        Material arrowYMaterial = Material({0.f, 255.f, 0.f, 255.f});
        Material arrowZMaterial = Material({0.f, 0.f, 255.f, 255.f});

        clm::vec3 clearColor;

        clm::vec3 cameraPos;
        clm::vec3 cameraRot;
        clm::mat4 viewMat;

        clm::vec3 surfaceToSunDir = {0.f, 1.f, 0.f};

        uint32_t selectedModelIndex = INVALID_INDEX;

        float dt;

        float vignetteStrength = 0.f;

        GLFWwindow *window;
        uint32_t windowWidth = 1000, windowHeight = 1000;

        [[nodiscard]] const std::vector<uint8_t> getImageRasterized();
        [[nodiscard]] const std::vector<uint8_t> getImageRayTraced(uint32_t width, uint32_t height);

        void updateCamera();

        uint32_t findHoveredModel(uint32_t mouseX, uint32_t mouseY);

        void castRay(const clm::vec3 &rayOrigin, const clm::vec3 &rayVector, uint32_t *outModelIndex, uint32_t *outMeshIndex, clm::vec3 *outNormal, clm::vec3 *outIntersectionPoint);
    };
}