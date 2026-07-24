// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

namespace CL
{
    class Camera
    {
    public:
        struct Basis
        {
            clm::vec3 forward;
            clm::vec3 right;
            clm::vec3 up;
        };

        void update(float dolly, float truck, float pan, float tilt, float dt)
        {
            const float speed = 0.001f;

            rot.y += pan * clm::PI * dt * speed;

            rot.x += tilt * clm::PI * dt * speed;
            rot.x = clm::clamp(rot.x, -clm::PI / 2, clm::PI / 2);

            Basis basis = getBasis();

            const float forwardScale = dolly * dt * speed;
            const clm::vec3 forwardScaled = basis.forward * forwardScale;

            const float leftScale = truck * dt * speed;
            const clm::vec3 rightScaled = basis.right * leftScale;

            const clm::vec3 delta = forwardScaled + rightScaled;

            pos += delta;

            mat = clm::mat4::rotation(-rot.x, 0.f, 0.f) * clm::mat4::rotation(0.f, -rot.y, 0.f) * clm::mat4::translation(-pos.x, -pos.y, -pos.z);
        }

        clm::mat4 viewMat() const { return mat; };

        Basis getBasis() const
        {
            return Basis{
                .forward = clm::vec3(cosf(rot.x) * sinf(rot.y), -sinf(rot.x), cosf(rot.x) * cosf(rot.y)),
                .right = clm::vec3(cosf(rot.y), 0.f, -sinf(rot.y)),
                .up = clm::vec3(sinf(rot.x) * sinf(rot.y), cosf(rot.x), sinf(rot.x) * cosf(rot.y))};
        }

        clm::vec3 getPos() const { return pos; }

    private:
        clm::vec3 pos;
        clm::vec3 rot;

        clm::mat4 mat;
    };
}