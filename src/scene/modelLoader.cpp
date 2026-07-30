// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "scene.hpp"

#include <unordered_map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <stdexcept>

namespace CL
{
    Model loadOBJ(const std::string &filePath)
    {
        std::string ext = std::filesystem::path(filePath).extension().string();
        if(ext != ".obj")
            throw std::runtime_error("File is not in .obj format: " + filePath);

        std::ifstream file(filePath);
        if (!file.is_open())
            throw std::runtime_error("File not found: " + filePath);

        std::vector<clm::vec3> positions;
        std::unordered_map<std::string, Material> materials;

        uint32_t currentMaterialIndex = INVALID_INDEX;

        std::vector<Vertex> currentMeshVertices;
        std::vector<uint32_t> currentMeshIndices;

        auto trim = [](std::string &s)
        {
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
            s.erase(0, s.find_first_not_of(" \t\r\n"));
        };

        auto loadMTL = [&](const std::string &mtlPath)
        {
            std::ifstream mtl(mtlPath);
            if (!mtl.is_open())
                return;

            std::string line;
            std::string currentMat;

            while (std::getline(mtl, line))
            {
                if (line.starts_with("newmtl "))
                {
                    currentMat = line.substr(7);
                    trim(currentMat);
                    materials[currentMat].color.w = 1.0f;
                }
                else if (line.starts_with("Kd ") && !currentMat.empty())
                {
                    std::stringstream ss(line.substr(3));
                    clm::vec3 kd;
                    ss >> kd.x >> kd.y >> kd.z;
                    materials[currentMat].color = {kd.x * 255.f, kd.y * 255.f, kd.z * 255.f, materials[currentMat].color.w};
                }
                else if (line.starts_with("d ") && !currentMat.empty())
                {
                    std::stringstream ss(line.substr(2));
                    float dissolve;
                    ss >> dissolve;
                    materials[currentMat].color.w = dissolve * 255.f;
                }
            }
        };

        std::vector<Mesh> meshes;
        std::vector<Material> materialList;
        std::unordered_map<std::string, uint32_t> materialIndexByName;

        auto finalizeCurrentMesh = [&]()
        {
            if (currentMeshVertices.empty())
                return;

            uint32_t resolvedMaterialIndex = currentMaterialIndex;
            if (resolvedMaterialIndex == INVALID_INDEX)
            {
                resolvedMaterialIndex = static_cast<uint32_t>(materialList.size());
                materialList.push_back(Material());
                currentMaterialIndex = resolvedMaterialIndex;
            }

            meshes.emplace_back(currentMeshVertices, currentMeshIndices, resolvedMaterialIndex);

            currentMeshVertices.clear();
            currentMeshIndices.clear();
        };

        std::filesystem::path objPath(filePath);

        std::string line;
        while (std::getline(file, line))
        {
            if (line.starts_with("mtllib "))
            {
                std::string mtlFile = line.substr(7);
                trim(mtlFile);
                loadMTL((objPath.parent_path() / mtlFile).string());
            }
            else if (line.starts_with("usemtl "))
            {
                finalizeCurrentMesh();

                std::string mat = line.substr(7);
                trim(mat);

                auto it = materials.find(mat);
                if (it != materials.end())
                {
                    auto idxIt = materialIndexByName.find(mat);
                    if (idxIt == materialIndexByName.end())
                    {
                        currentMaterialIndex = static_cast<uint32_t>(materialList.size());
                        materialList.push_back(it->second);
                        materialIndexByName.emplace(mat, currentMaterialIndex);
                    }
                    else
                    {
                        currentMaterialIndex = idxIt->second;
                    }
                }
            }
            else if (line.starts_with("v "))
            {
                clm::vec3 p;
                std::stringstream ss(line.substr(2));
                ss >> p.x >> p.y >> p.z;
                positions.push_back(p);
            }
            else if (line.starts_with("f "))
            {
                std::stringstream ss(line.substr(2));
                std::string a, b, c;
                ss >> a >> b >> c;

                auto parseIndex = [](const std::string &token) -> uint32_t
                {
                    return static_cast<uint32_t>(std::stoi(token.substr(0, token.find('/'))) - 1);
                };

                uint32_t parsed[3] = {parseIndex(a), parseIndex(b), parseIndex(c)};

                for (auto &positionIndex : parsed)
                {
                    currentMeshIndices.push_back(static_cast<uint32_t>(currentMeshVertices.size()));
                    currentMeshVertices.emplace_back(positions[positionIndex]);
                }
            }
            else if (line.starts_with("o "))
            {
                finalizeCurrentMesh();
            }
        }

        finalizeCurrentMesh();

        Model model(meshes, materialList);

        return model;
    }
}