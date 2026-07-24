// Copyright 2026 Emil Dimov
// Licensed under the Apache License, Version 2.0

#pragma once

#include "../math/clm.hpp"

#include <stdexcept>

#include <GLFW/glfw3.h>

namespace CL
{
    class Window
    {
    public:
        Window(clm::uvec2 size)
        {
            this->size = size;
            aspectRatio = static_cast<float>(size.x) / size.y;
            hasResized = true;

            if (!glfwInit())
                throw std::runtime_error("Failed to initialize GLFW");

            ptr = glfwCreateWindow(static_cast<int>(size.x), static_cast<int>(size.y), "Cull Engine - Editor", nullptr, nullptr);
            if (!ptr)
                throw std::runtime_error("Failed to create GLFW window");

            glfwMakeContextCurrent(ptr);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glfwSetWindowUserPointer(ptr, this);

            glfwSetFramebufferSizeCallback(ptr, [](GLFWwindow *window, int width, int height)
                                           {
            Window *w = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            w->size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
            w->aspectRatio = static_cast<float>(width) / height; 
            w->hasResized = true; });
        }

        ~Window()
        {
            if (ptr)
                glfwDestroyWindow(ptr);
            ptr = nullptr;

            glfwTerminate();
        }

        clm::uvec2 size;
        float aspectRatio;

        bool hasResized;

        bool isKeyPressed(uint32_t key)
        {
            return (glfwGetKey(ptr, key) == GLFW_PRESS);
        }

        bool isMouseBtnPressed(uint32_t btn)
        {
            return (glfwGetMouseButton(ptr, btn) == GLFW_PRESS);
        }

        operator GLFWwindow *()
        {
            return ptr;
        }

    private:
        GLFWwindow *ptr;
    };
}