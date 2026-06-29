// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

namespace CL
{
    Renderer::Renderer()
    {
    }

    void Renderer::tick()
    {
        for(Model model : models)
        {
            // Draw model
        }
    }

    void Renderer::addModel(Model &model)
    {
        models.push_back(model);
    }
}