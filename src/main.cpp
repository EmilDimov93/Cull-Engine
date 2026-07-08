// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

using namespace CL;

int main()
{
    Renderer renderer({0.f, 102.f, 0.f});

    Model car1 = renderer.loadOBJ("build/car/car.obj");
    car1.transform.pos = {0.f, -1.f, 8.f};
    car1.transform.rot = {0.f, 3.4f, 0.f};

    Model car2 = car1;
    car2.transform.pos.x = 5.f;

    renderer.addModel(car1);
    renderer.addModel(car2);

    renderer.setVignetteStrength(1.f);

    renderer.run();
}