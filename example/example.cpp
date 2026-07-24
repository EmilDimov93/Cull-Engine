// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "../src/editor/editor.hpp"

#include <iostream>

using namespace CL;

int main()
{
    try
    {
        Scene scene;
        scene.clearColor = {144.f, 213.f, 255.f};

        Model car = loadOBJ("example/assets/vw.obj");
        car.transform.pos = {1.7f, -1.f, 8.8f};
        car.transform.rot = clm::quaternion().rotate({0.f, clm::PI * 5.f / 4.f, 0.f});

        Model cow = loadOBJ("example/assets/cow.obj");
        cow.transform = Model::Transform({-1.8f, -1.f, 8.8f}, {0.f, clm::PI * 2.f / 3.f, 0.f}, {0.7f, 0.7f, 0.7f});

        Model ground = loadOBJ("example/assets/cube.obj");
        ground.transform.pos = {0.f, -1.3f, 8.8f};
        ground.transform.scale = {3.2f, 0.1f, 3.2f};

        scene.addModel(car);
        scene.addModel(cow);
        scene.addModel(ground);

        Editor renderer(scene, {1000, 1000});

        renderer.setVignetteStrength(1.f);

        renderer.setResultImageSize({10u, 10u});

        renderer.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}