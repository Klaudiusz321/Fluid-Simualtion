#include "../../include/render/Renderer.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>

Renderer::Renderer() {}

Renderer::~Renderer() {
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool Renderer::init(int width, int height, const char* title) {
    width_ = width;
    height_ = height;

    if (!glfwInit()) {
        std::cerr << "[Renderer] Failed to initialize GLFW\n";
        return false;
    }

    // Request OpenGL 4.3 core (for compute shaders later)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        std::cerr << "[Renderer] Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);

    // VSync
    glfwSwapInterval(1);

    // Load OpenGL function pointers via GLAD2
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "[Renderer] Failed to initialize GLAD\n";
        return false;
    }

    // Enable point sprites and blending
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, width, height);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);

    std::cout << "[Renderer] OpenGL " << glGetString(GL_VERSION) << "\n";
    return true;
}

bool Renderer::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void Renderer::pollEvents() {
    glfwPollEvents();
}

void Renderer::beginFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
    glfwSwapBuffers(window_);
}

float Renderer::getTime() const {
    return static_cast<float>(glfwGetTime());
}
