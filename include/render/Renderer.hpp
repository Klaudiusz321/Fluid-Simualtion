#pragma once

struct GLFWwindow;

/// Base renderer - manages OpenGL context, window, and basic drawing.
class Renderer {
public:
    Renderer();
    ~Renderer();

    /// Initialize GLFW window and OpenGL context
    bool init(int width, int height, const char* title);

    /// Returns true if the window should close
    bool shouldClose() const;

    /// Poll events
    void pollEvents();

    /// Clear the screen
    void beginFrame();

    /// Swap buffers
    void endFrame();

    /// Get window handle
    GLFWwindow* getWindow() const { return window_; }

    /// Get elapsed time since init
    float getTime() const;

    /// Get window dimensions
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    GLFWwindow* window_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};
