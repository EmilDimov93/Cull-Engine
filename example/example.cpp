// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "../src/editor/editor.hpp"

#include <iostream>

int main()
{
    try
    {
        CL::Scene scene;
        scene.clearColor = {144.f, 213.f, 255.f};

        CL::Model car = CL::loadOBJ("example/assets/vw.obj");
        car.transform.pos = {1.3f, -1.f, 8.8f};
        car.transform.rot = clm::quaternion().rotate({0.f, clm::PI * 5.f / 4.f, 0.f});
        scene.addModel(car);

        CL::Model cow = CL::loadOBJ("example/assets/cow.obj");
        cow.transform = CL::Model::Transform({-2.0f, -1.1f, 8.8f}, {0.f, clm::PI * 2.f / 3.f, 0.f}, {0.7f, 0.7f, 0.7f});
        scene.addModel(cow);

        CL::Model ground = CL::loadOBJ("example/assets/cube.obj");
        ground.transform.pos = {0.f, -1.3f, 8.8f};
        ground.transform.scale = {3.2f, 0.1f, 3.2f};
        scene.addModel(ground);

        CL::Editor editor(scene, {1000u, 1000u});

        editor.setVignetteStrength(0.8f);
        editor.setResultImageSize({1000u, 1000u});

        editor.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}