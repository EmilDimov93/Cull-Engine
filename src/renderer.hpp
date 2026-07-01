// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "clm.hpp"

#include <vector>
#include <cstdint>
#include <string>
#include <fstream>

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
        Renderer(clm::vec3 clearColor = {0.f, 0.f, 0.f}) : clearColor(clearColor) {}

        void renderModelsToImage(std::string filePath, uint32_t width, uint32_t height);

        void addModel(Model &model);
        [[nodiscard]] Model loadOBJ(const std::string &filePath);
    private:
        std::vector<Model> models;

        clm::vec3 clearColor;

        const std::vector<unsigned char> getImage(uint32_t width, uint32_t height);
        void uploadImage(std::vector<unsigned char> image, std::string filePath, uint32_t width, uint32_t height);
    };
}