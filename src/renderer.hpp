// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

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

        Material(clm::vec4 color = {1.f, 1.f, 1.f, 1.f}) : color(color) {}
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

        clm::mat4 transform;

        Model(const std::vector<Mesh> &meshes, const std::vector<Material> &materials, clm::mat4 transform = clm::mat4()) : meshes(meshes), materials(materials), transform(transform) {}
    };

    class Renderer
    {
    public:
        Renderer(clm::vec3 clearColor = {0.f, 0.f, 0.f});
        ~Renderer();

        void addModel(Model &model);
        [[nodiscard]] Model loadOBJ(const std::string &filePath);

        void setVignetteStrength(float vignetteStrength) { this->vignetteStrength = vignetteStrength; }

        void setSunDir(clm::vec3 sunToSurfaceDir) { surfaceToSunDir = sunToSurfaceDir.normalized(); surfaceToSunDir = surfaceToSunDir * -1; }

        void run();

    private:
        static constexpr float FOV = clm::PI / 3;

        std::vector<Model> models;

        clm::vec3 clearColor;

        clm::vec3 cameraPos;

        clm::vec3 surfaceToSunDir = {0.f, 1.f, 0.f};

        float dt;

        float vignetteStrength = 0.f;

        GLFWwindow *window;
        uint32_t windowWidth = 1000, windowHeight = 1000;

        [[nodiscard]] const std::vector<uint8_t> getImageRasterized();
        [[nodiscard]] const std::vector<uint8_t> getImageRayTraced(uint32_t width, uint32_t height);
    };
}