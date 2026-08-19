// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

#include "camera.hpp"

#include <vector>
#include <string>
#include <limits>
#include <stdexcept>

namespace CL
{
    static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

    struct Vertex
    {
        clm::vec3 pos;
        clm::vec3 normal;
        clm::uvec2 tex;

        constexpr Vertex() = default;
        constexpr Vertex(clm::vec3 pos, clm::vec3 normal, clm::uvec2 tex = {}) : pos(pos), normal(normal), tex(tex) {}
    };

    struct Material
    {
        clm::vec4 color;
        float roughness;
        float metallic;
        float ior;
        std::vector<uint8_t> texturePixels;
        clm::uvec2 textureSize;

        constexpr Material(clm::vec4 color = {255.f, 255.f, 255.f, 255.f}, float roughness = 0.f, float metallic = 0.f) : color(color), roughness(roughness), metallic(metallic) {}
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        uint32_t materialIndex;

        Mesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> indices, uint32_t materialIndex) : vertices(vertices), indices(indices), materialIndex(materialIndex) {}
    };

    struct Model
    {
        std::vector<Mesh> meshes;
        std::vector<Material> materials;

        struct Transform
        {
            clm::vec3 pos;
            clm::quaternion rot;
            clm::vec3 scale;

            clm::mat4 mat() const
            {
                return clm::mat4::translation(pos.x, pos.y, pos.z) * rot.mat() * clm::mat4::scale(scale.x, scale.y, scale.z);
            };

            Transform(clm::vec3 pos = {}, clm::vec3 rot = {}, clm::vec3 scale = {1.f, 1.f, 1.f}) : pos(pos), rot(rot), scale(scale) {}
        } transform;

        Model(const std::vector<Mesh> &meshes, const std::vector<Material> &materials, Transform transform = Transform()) : meshes(meshes), materials(materials), transform(transform) {}
        Model() {}
    };

    struct PointLight
    {
        clm::vec3 pos;
        clm::vec3 color;
        float intensity;

        PointLight(clm::vec3 pos, clm::vec3 color, float intensity) : pos(pos), color(color), intensity(intensity) {}
    };

    [[nodiscard]] Model loadOBJ(const std::string &filePath);

    struct Scene
    {
        std::vector<Model> models;
        std::vector<PointLight> lights;
        Camera camera;
        clm::vec3 clearColor;
        clm::vec3 surfaceToSunDir = {0.f, 1.f, 0.f};
        float sunLightIntensity = 1.0f;
        clm::vec3 sunLightColor = {1.f, 1.f, 1.f};
        float ambient = 0.3f;

        uint32_t addModel(const Model &model)
        {
            models.push_back(model);
            return static_cast<uint32_t>(models.size() - 1);
        }

        void removeModel(uint32_t index)
        {
            if (index > models.size() - 1)
                throw std::runtime_error("Invalid index");
            models.erase(models.begin() + index);
        }
    };
}