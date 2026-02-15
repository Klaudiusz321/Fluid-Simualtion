#include "../include/core/SPHSolver.hpp"
#include "../include/render/Renderer.hpp"
#include "../include/render/FluidRenderer.hpp"
#include "../include/core/SimConfig.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

// ============================================================
// Fluid Simulation — Main Entry Point
// ============================================================
//
// SPH (Smoothed Particle Hydrodynamics) fluid simulator
// with spatial hashing, surface tension, XSPH correction,
// and screen-space fluid rendering.
//
// Controls:
//   Left Click  — Emit particles
//   R           — Reset simulation
//   SPACE       — Pause/Resume
//   1           — Point rendering mode
//   2           — Metaball (fluid surface) rendering mode
//   ESC         — Quit
// ============================================================

enum class RenderMode { Points, FluidSurface };

int main() {
    // --- Init Window ---
    Renderer renderer;
    int width  = static_cast<int>(SimConfig::WINDOW_WIDTH);
    int height = static_cast<int>(SimConfig::WINDOW_HEIGHT);

    if (!renderer.init(width, height, "SPH Fluid Simulation")) {
        return -1;
    }

    // --- Init Fluid Renderer ---
    FluidRenderer fluidRenderer;
    if (!fluidRenderer.init(width, height)) {
        return -1;
    }
    fluidRenderer.setFluidColor(0.15f, 0.45f, 0.95f, 0.85f);

    // --- Init Solver ---
    SPHSolver solver;
    solver.initDamBreak(40, 40);  // 1600 particles

    // --- State ---
    RenderMode renderMode = RenderMode::Points;
    bool paused = false;
    float lastTime = renderer.getTime();
    int frameCount = 0;
    float fpsTimer = 0.0f;

    std::cout << "\n=== SPH Fluid Simulation ===\n";
    std::cout << "Particles: " << solver.numParticles() << "\n";
    std::cout << "Controls:\n";
    std::cout << "  Left Click — Emit particles\n";
    std::cout << "  R          — Reset simulation\n";
    std::cout << "  SPACE      — Pause/Resume\n";
    std::cout << "  1          — Point rendering\n";
    std::cout << "  2          — Fluid surface rendering\n";
    std::cout << "  ESC        — Quit\n\n";

    // --- Main Loop ---
    while (!renderer.shouldClose()) {
        renderer.pollEvents();
        GLFWwindow* win = renderer.getWindow();

        float currentTime = renderer.getTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // FPS counter
        frameCount++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0f) {
            std::string title = "SPH Fluid — " + std::to_string(frameCount) + " FPS — " +
                                std::to_string(solver.numParticles()) + " particles";
            glfwSetWindowTitle(win, title.c_str());
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        // --- Input ---
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(win, true);
        }
        if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) {
            // Simple toggle with debounce
            static float lastToggle = 0.0f;
            if (currentTime - lastToggle > 0.3f) {
                paused = !paused;
                lastToggle = currentTime;
                std::cout << (paused ? "[Paused]" : "[Running]") << "\n";
            }
        }
        if (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS) {
            solver.initDamBreak(40, 40);
        }
        if (glfwGetKey(win, GLFW_KEY_1) == GLFW_PRESS) {
            renderMode = RenderMode::Points;
        }
        if (glfwGetKey(win, GLFW_KEY_2) == GLFW_PRESS) {
            renderMode = RenderMode::FluidSurface;
        }

        // Emit particles on mouse click
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double mx, my;
            glfwGetCursorPos(win, &mx, &my);
            // Emit a small burst
            for (int i = 0; i < 3; ++i) {
                float jx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 10.0f;
                float jy = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 10.0f;
                solver.addParticle(static_cast<float>(mx) + jx,
                                   static_cast<float>(my) + jy);
            }
        }

        // --- Physics ---
        if (!paused) {
            // Run multiple sub-steps for stability
            int subSteps = 3;
            for (int s = 0; s < subSteps; ++s) {
                solver.update();
            }
        }

        // --- Render ---
        renderer.beginFrame();

        fluidRenderer.updateParticles(solver.getParticles());

        switch (renderMode) {
            case RenderMode::Points:
                fluidRenderer.renderPoints(10.0f);
                break;
            case RenderMode::FluidSurface:
                fluidRenderer.renderFluidSurface();
                break;
        }

        renderer.endFrame();
    }

    fluidRenderer.cleanup();
    std::cout << "[Exit] Simulation ended.\n";
    return 0;
}
