// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "model.hpp"

namespace CL
{
    class Renderer
    {
    public:
        Renderer();

        void tick();

        void addModel(Model &model);
    private:
        std::vector<Model> models;
    };
}