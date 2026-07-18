// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

#include <cstdlib>
#include <chrono>
#include <iostream>

namespace CL
{
    Renderer::Renderer(clm::vec2 windowSize, clm::vec3 clearColor) : clearColor(clearColor)
    {
        this->windowSize = windowSize;
        aspectRatio = windowSize.x / windowSize.y;
        projectionMat = clm::perspectiveProjectionMat(FOV, aspectRatio, 0.001f, 1000.f);

        if (!glfwInit())
            exit(1);

        window = glfwCreateWindow(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y), "Cull Engine - Editor", nullptr, nullptr);
        if (!window)
            exit(1);

        glfwMakeContextCurrent(window);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glfwSetWindowUserPointer(window, this);

        glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height)
                                       {
            Renderer *r = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
            r->windowSize = {static_cast<float>(width), static_cast<float>(height)};
            r->aspectRatio = r->windowSize.x / r->windowSize.y;
            r->projectionMat = clm::perspectiveProjectionMat(FOV, r->aspectRatio, 0.001f, 1000.f); });

        gizmoArrow = loadOBJ("assets/gizmo_arrow.obj");
    }

    Renderer::~Renderer()
    {
        if (window)
            glfwDestroyWindow(window);
        window = nullptr;

        glfwTerminate();
    }

    void Renderer::runEditor()
    {
        while (!glfwWindowShouldClose(window))
        {
            const auto startTime = std::chrono::steady_clock::now();

            glfwPollEvents();

            updateCamera();

            std::vector<uint8_t> image = getImageRasterized();

            glDrawPixels(static_cast<GLsizei>(windowSize.x), static_cast<GLsizei>(windowSize.y), GL_RGB, GL_UNSIGNED_BYTE, image.data());
            glfwSwapBuffers(window);

            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);

            if ((glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS))
            {
                if (!wasGPressed)
                {
                    wasGPressed = true;
                    gizmoMode = static_cast<GizmoMode>((gizmoMode + 1) % 3);
                }
            }
            else
            {
                wasGPressed = false;
            }

            constexpr float dragSensitivity = 0.01f;
            if ((glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS))
            {
                const float deltaX = static_cast<float>(mouseX) - prevMousePos.x;
                const float deltaY = static_cast<float>(mouseY) - prevMousePos.y;

                switch (gizmoDrag)
                {
                case GIZMO_DRAG_NONE:
                    selectedModelIndex = findHoveredModel(mouseX, mouseY);
                    break;
                case GIZMO_DRAG_X_ARROW:
                    switch (gizmoMode)
                    {
                    case GIZMO_MODE_TRANSLATE:
                        models[selectedModelIndex].transform.pos.x += deltaX * dragSensitivity;
                        break;
                    case GIZMO_MODE_ROTATE:
                        models[selectedModelIndex].transform.rot = models[selectedModelIndex].transform.rot.rotate({0.f, deltaX * dragSensitivity, 0.f});
                        break;
                    case GIZMO_MODE_SCALE:
                        models[selectedModelIndex].transform.scale.x += deltaX * dragSensitivity;
                        break;
                    }
                    break;
                case GIZMO_DRAG_Y_ARROW:
                    switch (gizmoMode)
                    {
                    case GIZMO_MODE_TRANSLATE:
                        models[selectedModelIndex].transform.pos.y += -deltaY * dragSensitivity;
                        break;
                    case GIZMO_MODE_ROTATE:
                        models[selectedModelIndex].transform.rot = models[selectedModelIndex].transform.rot.rotate({0.f, 0.f, -deltaY * dragSensitivity});
                        break;
                    case GIZMO_MODE_SCALE:
                        models[selectedModelIndex].transform.scale.y += -deltaY * dragSensitivity;
                        break;
                    }
                    break;
                case GIZMO_DRAG_Z_ARROW:
                    switch (gizmoMode)
                    {
                    case GIZMO_MODE_TRANSLATE:
                        models[selectedModelIndex].transform.pos.z += deltaX * dragSensitivity;
                        break;
                    case GIZMO_MODE_ROTATE:
                        models[selectedModelIndex].transform.rot = models[selectedModelIndex].transform.rot.rotate({deltaX * dragSensitivity, 0.f, 0.f});
                        break;
                    case GIZMO_MODE_SCALE:
                        models[selectedModelIndex].transform.scale.z += deltaX * dragSensitivity;
                        break;
                    }
                    break;
                }
            }
            else
            {
                gizmoDrag = GIZMO_DRAG_NONE;
            }

            prevMousePos = {static_cast<float>(mouseX), static_cast<float>(mouseY)};

            if ((glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS))
                editorViewMode = EDITOR_VIEW_SOLID;
            else
                editorViewMode = EDITOR_VIEW_WIREFRAME;

            if ((glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS) && (selectedModelIndex != INVALID_INDEX))
            {
                models.erase(models.begin() + selectedModelIndex);
                selectedModelIndex = INVALID_INDEX;
            }

            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
                renderToPPM({100.f, 100.f});

            dt = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
            std::cout << 1000.f / dt << std::endl;
        }
    }

    void Renderer::renderToPPM(clm::vec2 imageSize)
    {
        std::vector<uint8_t> image = getImageRayTraced(imageSize);

        std::ofstream file("build/result.ppm", std::ios::binary);

        file << "P6\n"
             << static_cast<uint32_t>(imageSize.x) << ' ' << static_cast<uint32_t>(imageSize.y) << "\n255\n";

        file.write(reinterpret_cast<const char *>(image.data()), static_cast<std::streamsize>(imageSize.x) * static_cast<std::streamsize>(imageSize.y) * 3);

        file.close();

        std::system("start build\\result.ppm");
    }

    void Renderer::addModel(Model &model)
    {
        models.push_back(model);
    }

    void Renderer::updateCamera()
    {
        const float speed = 0.001f;

        cameraRot.y += ((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)) * clm::PI * dt * speed;

        cameraRot.x += ((glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)) * clm::PI * dt * speed;
        cameraRot.x = std::clamp(cameraRot.x, -clm::PI / 2, clm::PI / 2);

        const float forwardBackward = ((glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)) * dt;
        const clm::vec3 forward = {cosf(cameraRot.x) * sinf(cameraRot.y), -sinf(cameraRot.x), cosf(cameraRot.x) * cosf(cameraRot.y)};
        const clm::vec3 forwardScaled = forward * forwardBackward;

        const float leftRight = ((glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)) * dt;
        const clm::vec3 right = {cosf(cameraRot.y), 0.f, -sinf(cameraRot.y)};
        const clm::vec3 rightScaled = right * leftRight;

        const clm::vec3 delta = (forwardScaled + rightScaled) * speed;

        cameraPos += delta;

        viewMat = clm::rotationMat(-cameraRot.x, 0.f, 0.f) * clm::rotationMat(0.f, -cameraRot.y, 0.f) * clm::translationMat(-cameraPos.x, -cameraPos.y, -cameraPos.z);
    }

    uint32_t Renderer::findHoveredModel(uint32_t mouseX, uint32_t mouseY)
    {
        const float ndcX = clm::unitToSignedRange((mouseX + 0.5f) / windowSize.x) * aspectRatio * TAN_HALF_FOV;
        const float ndcY = -clm::unitToSignedRange((mouseY + 0.5f) / windowSize.y) * TAN_HALF_FOV;

        const clm::vec3 forward = {cosf(cameraRot.x) * sinf(cameraRot.y), -sinf(cameraRot.x), cosf(cameraRot.x) * cosf(cameraRot.y)};
        const clm::vec3 right = {cosf(cameraRot.y), 0.f, -sinf(cameraRot.y)};
        const clm::vec3 up = {sinf(cameraRot.x) * sinf(cameraRot.y), cosf(cameraRot.x), sinf(cameraRot.x) * cosf(cameraRot.y)};

        const clm::vec3 rayDirection = (right * ndcX + up * ndcY + forward).normalized();

        uint32_t hitModelIndex = INVALID_INDEX;

        if (selectedModelIndex != INVALID_INDEX)
        {
            std::vector<Model> gizmoArrows{gizmoArrow};

            gizmoArrows[0].transform = Model::Transform(models[selectedModelIndex].transform.pos, {0.f, 0.f, -clm::PI / 2}, {0.4f, 0.4f, 0.4f});
            castRay(gizmoArrows, cameraPos, rayDirection, &hitModelIndex, nullptr, nullptr, nullptr);

            if (hitModelIndex != INVALID_INDEX)
            {
                gizmoDrag = GIZMO_DRAG_X_ARROW;
                return selectedModelIndex;
            }

            gizmoArrows[0].transform = Model::Transform(models[selectedModelIndex].transform.pos, {0.f, 0.f, 0.f}, {0.4f, 0.4f, 0.4f});
            castRay(gizmoArrows, cameraPos, rayDirection, &hitModelIndex, nullptr, nullptr, nullptr);

            if (hitModelIndex != INVALID_INDEX)
            {
                gizmoDrag = GIZMO_DRAG_Y_ARROW;
                return selectedModelIndex;
            }

            gizmoArrows[0].transform = Model::Transform(models[selectedModelIndex].transform.pos, {-clm::PI / 2, 0.f, 0.f}, {0.4f, 0.4f, 0.4f});
            castRay(gizmoArrows, cameraPos, rayDirection, &hitModelIndex, nullptr, nullptr, nullptr);

            if (hitModelIndex != INVALID_INDEX)
            {
                gizmoDrag = GIZMO_DRAG_Z_ARROW;
                return selectedModelIndex;
            }
        }

        castRay(models, cameraPos, rayDirection, &hitModelIndex, nullptr, nullptr, nullptr);

        return hitModelIndex;
    }
}