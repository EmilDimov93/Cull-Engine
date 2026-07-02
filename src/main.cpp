// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

using namespace CL;

#include <chrono>
#include <iostream>

int main()
{
    Renderer renderer({0.f, 102.f, 0.f});

    auto startTime = std::chrono::steady_clock::now();

    Model model = renderer.loadOBJ("build/car/car.obj");
    model.transform = clm::mat4() * clm::translationMat(0.f, -0.5f, 3.f) * clm::rotationMat(0.f, 3.4f, 0.f) * clm::scaleMat(0.5f);

    renderer.addModel(model);

    auto loadTime = std::chrono::steady_clock::now();
    std::cout << "load: " << std::chrono::duration_cast<std::chrono::milliseconds>(loadTime - startTime).count() << std::endl;

    renderer.renderModelsToImage("build/rasterized.ppm", 500, 500, RENDER_MODE_RASTERIZATION);

    auto rasterizeTime = std::chrono::steady_clock::now();
    std::cout << "rasterize: " << std::chrono::duration_cast<std::chrono::milliseconds>(rasterizeTime - loadTime).count() << std::endl;

    renderer.renderModelsToImage("build/raytraced.ppm", 500, 500, RENDER_MODE_RAY_TRACING);

    auto raytraceTime = std::chrono::steady_clock::now();
    std::cout << "raytrace: " << std::chrono::duration_cast<std::chrono::milliseconds>(raytraceTime - rasterizeTime).count() << std::endl;
}