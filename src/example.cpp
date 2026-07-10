// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

using namespace CL;

int main()
{
    Renderer renderer({1000.f, 1000.f}, {144.f, 213.f, 255.f});

    Model car = renderer.loadOBJ("build/vw/vw.obj");
    car.transform.pos = {2.f, -1.f, 8.f};
    car.transform.rot = {0.f, clm::PI * 5.f / 4.f, 0.f};

    Model cow = renderer.loadOBJ("build/cow.obj");
    cow.transform = Model::Transform({-1.5f, -1.f, 8.f}, {0.f, clm::PI * 2.f / 3.f, 0.f}, {0.7f, 0.7f, 0.7f});

    Model ground = renderer.loadOBJ("build/cube.obj");
    ground.transform.pos = {0.f, -1.3f, 8.f};
    ground.transform.scale = {5.f, 0.1f, 5.f};

    renderer.addModel(car);
    renderer.addModel(cow);
    renderer.addModel(ground);

    renderer.setVignetteStrength(1.f);

    renderer.run();
}