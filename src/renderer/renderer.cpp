// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

namespace CL
{
    Renderer::Renderer(clm::uvec2 windowSize, clm::vec3 clearColor) : clearColor(clearColor)
    {
        this->windowSize = windowSize;
        aspectRatio = static_cast<float>(windowSize.x) / windowSize.y;
        projectionMat = clm::mat4::perspective(FOV, aspectRatio, ZNEAR, ZFAR);

        colorAttachmentMain.resize(windowSize.x * windowSize.y * 3, 0u);
        depthAttachmentMain.resize(windowSize.x * windowSize.y, std::numeric_limits<float>::infinity());
        depthAttachmentGizmo.resize(windowSize.x * windowSize.y, std::numeric_limits<float>::infinity());

        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");

        window = glfwCreateWindow(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y), "Cull Engine - Editor", nullptr, nullptr);
        if (!window)
            throw std::runtime_error("Failed to create GLFW window");

        glfwMakeContextCurrent(window);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glfwSetWindowUserPointer(window, this);

        glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height)
                                       {
            Renderer *r = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
            r->windowSize = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
            r->aspectRatio = static_cast<float>(width) / height;
            r->projectionMat = clm::mat4::perspective(FOV, r->aspectRatio, ZNEAR, ZFAR);
        
            r->colorAttachmentMain.resize(width * height * 3, 0u);
            r->depthAttachmentMain.resize(width * height, std::numeric_limits<float>::infinity());
            r->depthAttachmentGizmo.resize(width * height, std::numeric_limits<float>::infinity()); });

        gizmoArrow = loadOBJ("assets/gizmo_arrow.obj");
    }

    Renderer::~Renderer()
    {
        if (window)
            glfwDestroyWindow(window);
        window = nullptr;

        glfwTerminate();
    }

    void Renderer::renderToPPM(clm::uvec2 imageSize)
    {
        std::vector<uint8_t> image = getImageRayTraced(imageSize);

        std::ofstream file("build/result.ppm", std::ios::binary);

        if (!file)
            throw std::runtime_error("Failed to open build/result.ppm");

        file << "P6\n"
             << imageSize.x << ' ' << imageSize.x << "\n255\n";

        file.write(reinterpret_cast<const char *>(image.data()), static_cast<std::streamsize>(imageSize.x) * static_cast<std::streamsize>(imageSize.y) * 3);

        file.close();

        std::system("start build\\result.ppm");
    }

    void Renderer::addModel(Model &model)
    {
        models.push_back(std::move(model));
    }

    void Renderer::Camera::update(GLFWwindow *window, float dt)
    {
        const float speed = 0.001f;

        rot.y += ((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)) * clm::PI * dt * speed;

        rot.x += ((glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)) * clm::PI * dt * speed;
        rot.x = std::clamp(rot.x, -clm::PI / 2, clm::PI / 2);

        Basis basis = getBasis();

        const float forwardScale = ((glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)) * dt * speed;
        const clm::vec3 forwardScaled = basis.forward * forwardScale;

        const float leftScale = ((glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)) * dt * speed;
        const clm::vec3 rightScaled = basis.right * leftScale;

        const clm::vec3 delta = forwardScaled + rightScaled;

        pos += delta;

        mat = clm::mat4::rotation(-rot.x, 0.f, 0.f) * clm::mat4::rotation(0.f, -rot.y, 0.f) * clm::mat4::translation(-pos.x, -pos.y, -pos.z);
    }
}