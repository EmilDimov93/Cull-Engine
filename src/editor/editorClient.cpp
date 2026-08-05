// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#include "editor.hpp"

#include <cstdlib>
#include <chrono>

namespace CL
{
    Editor::Editor(const Scene &scene, clm::uvec2 windowSize) : window(windowSize), colorAttachmentMain(windowSize), depthAttachmentMain(windowSize), depthAttachmentGizmo(windowSize)
    {
        this->scene = scene;

        gizmoArrow = loadOBJ("assets/gizmo_arrow.obj");
    }

    void Editor::run()
    {
        while (!glfwWindowShouldClose(window))
        {
            const auto startTime = std::chrono::steady_clock::now();

            glfwPollEvents();

            if (window.hasResized)
            {
                projectionMat = clm::mat4::perspective(FOV, window.aspectRatio, ZNEAR, ZFAR);

                colorAttachmentMain.resize(window.size, 0u);
                depthAttachmentMain.resize(window.size, std::numeric_limits<float>::infinity());
                depthAttachmentGizmo.resize(window.size, std::numeric_limits<float>::infinity());

                window.hasResized = false;
            }

            scene.camera.update((window.isKeyPressed(GLFW_KEY_W) - window.isKeyPressed(GLFW_KEY_S)),
                                (window.isKeyPressed(GLFW_KEY_D) - window.isKeyPressed(GLFW_KEY_A)),
                                (window.isKeyPressed(GLFW_KEY_RIGHT) - window.isKeyPressed(GLFW_KEY_LEFT)),
                                (window.isKeyPressed(GLFW_KEY_DOWN) - window.isKeyPressed(GLFW_KEY_UP)),
                                dt);

            renderSceneRasterized();

            glDrawPixels(static_cast<GLsizei>(window.size.x), static_cast<GLsizei>(window.size.y), GL_RGB, GL_UNSIGNED_BYTE, colorAttachmentMain.image.data());
            glfwSwapBuffers(window);

            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);

            if (window.isKeyPressed(GLFW_KEY_G))
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
            if (window.isMouseBtnPressed(GLFW_MOUSE_BUTTON_LEFT))
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
                        scene.models[selectedModelIndex].transform.pos.x += deltaX * dragSensitivity;
                        break;
                    case GIZMO_MODE_ROTATE:
                        scene.models[selectedModelIndex].transform.rot = scene.models[selectedModelIndex].transform.rot.rotate({0.f, deltaX * dragSensitivity, 0.f});
                        break;
                    case GIZMO_MODE_SCALE:
                        scene.models[selectedModelIndex].transform.scale.x += deltaX * dragSensitivity;
                        break;
                    }
                    break;
                case GIZMO_DRAG_Y_ARROW:
                    switch (gizmoMode)
                    {
                    case GIZMO_MODE_TRANSLATE:
                        scene.models[selectedModelIndex].transform.pos.y += -deltaY * dragSensitivity;
                        break;
                    case GIZMO_MODE_ROTATE:
                        scene.models[selectedModelIndex].transform.rot = scene.models[selectedModelIndex].transform.rot.rotate({0.f, 0.f, -deltaY * dragSensitivity});
                        break;
                    case GIZMO_MODE_SCALE:
                        scene.models[selectedModelIndex].transform.scale.y += -deltaY * dragSensitivity;
                        break;
                    }
                    break;
                case GIZMO_DRAG_Z_ARROW:
                    switch (gizmoMode)
                    {
                    case GIZMO_MODE_TRANSLATE:
                        scene.models[selectedModelIndex].transform.pos.z += deltaX * dragSensitivity;
                        break;
                    case GIZMO_MODE_ROTATE:
                        scene.models[selectedModelIndex].transform.rot = scene.models[selectedModelIndex].transform.rot.rotate({deltaX * dragSensitivity, 0.f, 0.f});
                        break;
                    case GIZMO_MODE_SCALE:
                        scene.models[selectedModelIndex].transform.scale.z += deltaX * dragSensitivity;
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

            if (window.isKeyPressed(GLFW_KEY_E))
                editorViewMode = EDITOR_VIEW_SOLID;
            else
                editorViewMode = EDITOR_VIEW_WIREFRAME;

            if (window.isKeyPressed(GLFW_KEY_DELETE) && (selectedModelIndex != INVALID_INDEX))
            {
                scene.removeModel(selectedModelIndex);
                selectedModelIndex = INVALID_INDEX;
            }

            if (window.isKeyPressed(GLFW_KEY_R))
            {
                glfwSetWindowTitle(window, ("Cull Engine | Rendering..."));
                saveImageToPPM(renderSceneRayTraced(resultImageSize, scene, FOV, vignetteStrength, bvhDepth), resultImageSize, "build/result.ppm", true);
            }

            dt = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
            glfwSetWindowTitle(window, ("Cull Engine - Editor | " + std::to_string(int(1000.f / dt)) + " FPS").c_str());
        }
    }

    uint32_t Editor::findHoveredModel(clm::ivec2 mousePos)
    {
        const float ndcX = clm::unitToSignedRange((mousePos.x + 0.5f) / window.size.x) * window.aspectRatio * TAN_HALF_FOV;
        const float ndcY = -clm::unitToSignedRange((mousePos.y + 0.5f) / window.size.y) * TAN_HALF_FOV;

        Camera::Basis basis = scene.camera.getBasis();

        const clm::vec3 rayDirection = (basis.right * ndcX + basis.up * ndcY + basis.forward).normalized();

        uint32_t hitModelIndex = INVALID_INDEX;

        clm::vec3 cameraPos = scene.camera.getPos();

        if (selectedModelIndex != INVALID_INDEX)
        {
            std::vector<Model> gizmoArrows{gizmoArrow};

            gizmoArrows[0].transform = Model::Transform(scene.models[selectedModelIndex].transform.pos, {0.f, 0.f, -clm::PI / 2}, {0.4f, 0.4f, 0.4f});
            castRay(gizmoArrows, cameraPos, rayDirection, &hitModelIndex, nullptr, nullptr, nullptr);

            if (hitModelIndex != INVALID_INDEX)
            {
                gizmoDrag = GIZMO_DRAG_X_ARROW;
                return selectedModelIndex;
            }

            gizmoArrows[0].transform = Model::Transform(scene.models[selectedModelIndex].transform.pos, {0.f, 0.f, 0.f}, {0.4f, 0.4f, 0.4f});
            castRay(gizmoArrows, cameraPos, rayDirection, &hitModelIndex, nullptr, nullptr, nullptr);

            if (hitModelIndex != INVALID_INDEX)
            {
                gizmoDrag = GIZMO_DRAG_Y_ARROW;
                return selectedModelIndex;
            }

            gizmoArrows[0].transform = Model::Transform(scene.models[selectedModelIndex].transform.pos, {-clm::PI / 2, 0.f, 0.f}, {0.4f, 0.4f, 0.4f});
            castRay(gizmoArrows, cameraPos, rayDirection, &hitModelIndex, nullptr, nullptr, nullptr);

            if (hitModelIndex != INVALID_INDEX)
            {
                gizmoDrag = GIZMO_DRAG_Z_ARROW;
                return selectedModelIndex;
            }
        }

        castRay(scene.models, cameraPos, rayDirection, &hitModelIndex, nullptr, nullptr, nullptr);

        return hitModelIndex;
    }
}