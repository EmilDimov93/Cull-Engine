// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

#include <iostream>

using namespace CL;

int main()
{
    std::cout << "Started" << std::endl;

    Renderer renderer;

    std::vector<Vertex> vertices = {{{45.f, 30.f, 0.f}},
                                    {{30.f, 60.f, 0.f}},
                                    {{60.f, 60.f, 0.f}}};

    std::vector<uint32_t> indices = {0, 1, 2};

    std::vector<Material> materials;
    materials.emplace_back(clm::vec4(255.f, 0.f, 0.f, 255.f));

    std::vector<Mesh> meshes;
    meshes.emplace_back(vertices, indices, 0);

    Model model(meshes, materials, clm::mat4());

    renderer.addModel(model);

    renderer.renderModelsToImage("build/img.ppm");

    std::cout << "Finished";
}