// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

using namespace CL;

int main()
{
    Renderer renderer({0.f, 102.f, 0.f});

    Model model = renderer.loadOBJ("build/car/car.obj");
    model.transform = clm::mat4() * clm::translationMat(0.f, -0.5f, 3.f) * clm::rotationMat(0.f, 3.4f, 0.f) * clm::scaleMat(0.5f);

    renderer.addModel(model);

    renderer.setVignetteStrength(1.f);

    renderer.renderModelsToImage("build/img.ppm", 500, 500, RENDER_MODE_RASTERIZATION);
}