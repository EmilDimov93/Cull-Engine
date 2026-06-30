// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

#include <iostream>

using namespace CL;

int main()
{
    std::cout << "Running";

    Renderer renderer;

    Vertex v1({0.f, 1.f, 0.f});
    Vertex v2({1.f, 0.f, 0.f});
    Vertex v3({1.f, 1.f, 0.f});
    std::vector<Vertex> vertices = {v1, v2, v3};

    std::vector<uint32_t> indices = {0, 1, 2};

    std::vector<Material> materials = {Material(clm::vec4(1.f, 0.f, 0.f, 1.f))};

    std::vector<Mesh> meshes = {Mesh(vertices, indices, 0)};

    Model model(meshes, materials, clm::mat4());

    renderer.addModel(model);

    renderer.renderModelsToImage("build/img.ppm", 100, 100);
}