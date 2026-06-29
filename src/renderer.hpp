// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "clm.hpp"

#include <vector>
#include <cstdint>

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

        Material(clm::vec4 color) : color(color) {}
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

        Model(const std::vector<Mesh> &meshes, const std::vector<Material> &materials, clm::mat4 transform) : meshes(meshes), materials(materials), transform(transform) {}
    };

    class Renderer
    {
    public:
        Renderer();

        void tick();

        void addModel(Model &model);
    private:
        std::vector<Model> models;
    };
}