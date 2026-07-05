// Copyright 2025 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

#include <cstdlib>
#include <chrono>
#include <iostream>
namespace CL
{
    Renderer::Renderer(clm::vec3 clearColor) : clearColor(clearColor)
    {
        if (!glfwInit())
            exit(1);

        window = glfwCreateWindow(windowWidth, windowHeight, "Ray-Tracer", nullptr, nullptr);
        if (!window)
            exit(1);

        glfwMakeContextCurrent(window);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glfwSetWindowUserPointer(window, this);

        glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height)
                                       {
            Renderer *r = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
            r->windowWidth = static_cast<uint32_t>(width);
            r->windowHeight = static_cast<uint32_t>(height); });
    }

    Renderer::~Renderer()
    {
        if (window)
            glfwDestroyWindow(window);
        window = nullptr;

        glfwTerminate();
    }

    void Renderer::run()
    {
        while (!glfwWindowShouldClose(window))
        {
            auto startTime = std::chrono::steady_clock::now();

            glfwPollEvents();

            std::vector<uint8_t> image = getImageRasterized();

            glPixelZoom(1.0f, -1.0f);
            glRasterPos2f(-1.0f, 1.0f);
            glDrawPixels(windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, image.data());
            glfwSwapBuffers(window);

            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            {
                constexpr uint32_t imageSize = 100;

                std::vector<uint8_t> image = getImageRayTraced(imageSize, imageSize);

                std::ofstream file("build/img.ppm", std::ios::binary);

                file << "P6\n"
                     << imageSize << ' ' << imageSize << "\n255\n";

                file.write(reinterpret_cast<const char *>(image.data()), static_cast<std::streamsize>(imageSize) * imageSize * 3);

                file.close();

                int exitCode = std::system("start build\\img.ppm");
            }

            dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
            std::cout << 1000.f / dt << std::endl;
        }
    }

    void Renderer::addModel(Model &model)
    {
        models.push_back(model);
    }
}