// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

using namespace CL;

int main()
{
    Renderer renderer({0.f, 102.f, 0.f});

    Model model = renderer.loadOBJ("build/car/car.obj");
    model.transform.pos = {0.f, -1.f, 8.f};
    model.transform.rot = {0.f, 3.4f, 0.f};

    Model m2 = model;
    m2.transform.pos.x = 5.f;

    renderer.addModel(model);
    renderer.addModel(m2);

    renderer.setVignetteStrength(1.f);

    renderer.run();
}