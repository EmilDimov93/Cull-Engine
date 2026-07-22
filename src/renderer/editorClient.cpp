// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "renderer.hpp"

#include <cstdlib>
#include <chrono>

namespace CL
{
    void Renderer::runEditor()
    {
        while (!glfwWindowShouldClose(window))
        {
            const auto startTime = std::chrono::steady_clock::now();

            glfwPollEvents();

            camera.update(window, dt);

            renderImageRasterized();

            glDrawPixels(static_cast<GLsizei>(windowSize.x), static_cast<GLsizei>(windowSize.y), GL_RGB, GL_UNSIGNED_BYTE, colorAttachmentMain.data());
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
                    selectedModelIndex = findHoveredModel(clm::ivec2(static_cast<int32_t>(mouseX), static_cast<int32_t>(mouseY)));
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

            prevMousePos = {static_cast<int32_t>(mouseX), static_cast<int32_t>(mouseY)};

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
                renderToPPM(resultImageSize);

            dt = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
            glfwSetWindowTitle(window, ("Cull Engine - Editor | " + std::to_string(int(1000.f / dt)) + " FPS").c_str());
        }
    }

    uint32_t Renderer::findHoveredModel(clm::ivec2 mousePos)
    {
        const float ndcX = clm::unitToSignedRange((mousePos.x + 0.5f) / windowSize.x) * aspectRatio * TAN_HALF_FOV;
        const float ndcY = -clm::unitToSignedRange((mousePos.y + 0.5f) / windowSize.y) * TAN_HALF_FOV;

        Camera::Basis basis = camera.getBasis();

        const clm::vec3 rayDirection = (basis.right * ndcX + basis.up * ndcY + basis.forward).normalized();

        uint32_t hitModelIndex = INVALID_INDEX;

        clm::vec3 cameraPos = camera.getPos();

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