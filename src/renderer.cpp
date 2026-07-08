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

        arrow = loadOBJ("build/arrow.obj");
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

            updateCamera();

            std::vector<uint8_t> image = getImageRasterized();

            glDrawPixels(windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, image.data());
            glfwSwapBuffers(window);

            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            {
                double mouseX, mouseY;
                glfwGetCursorPos(window, &mouseX, &mouseY);
                selectedModelIndex = findHoveredModel(mouseX, mouseY);
            }

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
            // std::cout << 1000.f / dt << std::endl;
        }
    }

    void Renderer::addModel(Model &model)
    {
        models.push_back(model);
    }

    void Renderer::updateCamera()
    {
        const float freeSpeed = 0.001f;

        cameraRot.y += ((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)) * clm::PI * dt * freeSpeed;

        cameraRot.x += ((glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)) * clm::PI * dt * freeSpeed;
        cameraRot.x = std::clamp(cameraRot.x, -clm::PI / 2, clm::PI / 2);

        float forwardBackward = ((glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)) * dt;
        clm::vec3 forward = {cosf(cameraRot.x) * sinf(cameraRot.y), -sinf(cameraRot.x), cosf(cameraRot.x) * cosf(cameraRot.y)};
        clm::vec3 forwardScaled = forward * forwardBackward;

        float leftRight = ((glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)) * dt;
        clm::vec3 right = {cosf(cameraRot.y), 0.f, -sinf(cameraRot.y)};
        clm::vec3 rightScaled = right * leftRight;

        clm::vec3 delta = (forwardScaled + rightScaled) * freeSpeed;

        cameraPos += delta;

        viewMat = clm::rotationMat(-cameraRot.x, 0.f, 0.f) * clm::rotationMat(0.f, -cameraRot.y, 0.f) * clm::translationMat(-cameraPos.x, -cameraPos.y, -cameraPos.z);
    }

    uint32_t Renderer::findHoveredModel(uint32_t mouseX, uint32_t mouseY)
    {
        const float tanHalfFov = std::tan(FOV * 0.5f);
        const float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

        const float ndcX = clm::unitToSignedRange((mouseX + 0.5f) / windowWidth) * aspectRatio * tanHalfFov;
        const float ndcY = -clm::unitToSignedRange((mouseY + 0.5f) / windowHeight) * tanHalfFov;

        const clm::vec3 forward = {cosf(cameraRot.x) * sinf(cameraRot.y), -sinf(cameraRot.x), cosf(cameraRot.x) * cosf(cameraRot.y)};
        const clm::vec3 right = {cosf(cameraRot.y), 0.f, -sinf(cameraRot.y)};
        const clm::vec3 up = {sinf(cameraRot.x) * sinf(cameraRot.y), cosf(cameraRot.x), sinf(cameraRot.x) * cosf(cameraRot.y)};

        const clm::vec3 rayDirection = (right * ndcX + up * ndcY + forward).normalized();

        uint32_t hitModelIndex = INVALID_INDEX;

        castRay(cameraPos, rayDirection, &hitModelIndex, nullptr, nullptr, nullptr);

        return hitModelIndex;
    }
}