#include "../include/core/SPHSolver.hpp"
#include "../include/render/Renderer.hpp"
#include "../include/render/FluidRenderer.hpp"
#include "../include/core/SimConfig.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

// ============================================================
// SPH Fluid Simulation — Main Entry Point
// ============================================================
//
// Controls:
//   1       — Dam-break scene
//   2       — Pool / fill scene
//   3       — Tall dam scene
//   R       — Reset current scene
//   SPACE   — Pause / Resume
//   ESC     — Quit
// ============================================================

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
    fluidRenderer.setFluidColor(0.1f, 0.4f, 0.9f, 0.9f);

    // --- Init Solver ---
    SPHSolver solver;

    int currentScene = 1;
    auto loadScene = [&](int scene) {
        currentScene = scene;
        switch (scene) {
            case 1: solver.initDamBreak(50, 50); break;      // ~2500 particles
            case 2: solver.initPool(0.30f);       break;     // fills bottom 30%
            case 3: solver.initTallDam();         break;     // tall column
            default: solver.initDamBreak(50, 50); break;
        }
        std::cout << "[Scene " << scene << "] " << solver.numParticles() << " particles\n";
    };
    loadScene(1);

    // --- State ---
    bool paused = false;
    float lastTime = renderer.getTime();
    int frameCount = 0;
    float fpsTimer = 0.0f;

    // Point-size enable (required for gl_PointSize in vertex shader)
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    std::cout << "\n=== SPH Fluid Simulation ===\n";
    std::cout << "Controls:\n";
    std::cout << "  1/2/3  — Switch scene (dam / pool / tall-dam)\n";
    std::cout << "  R      — Reset current scene\n";
    std::cout << "  SPACE  — Pause / Resume\n";
    std::cout << "  ESC    — Quit\n\n";

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
            std::string title = "SPH Fluid  |  " + std::to_string(frameCount) + " FPS  |  " +
                                std::to_string(solver.numParticles()) + " particles";
            glfwSetWindowTitle(win, title.c_str());
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        // --- Input ---
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(win, true);
        }

        // Pause/resume (debounced)
        if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) {
            static float lastToggle = 0.0f;
            if (currentTime - lastToggle > 0.3f) {
                paused = !paused;
                lastToggle = currentTime;
                std::cout << (paused ? "[Paused]" : "[Running]") << "\n";
            }
        }

        // Scene switching
        if (glfwGetKey(win, GLFW_KEY_1) == GLFW_PRESS) loadScene(1);
        if (glfwGetKey(win, GLFW_KEY_2) == GLFW_PRESS) loadScene(2);
        if (glfwGetKey(win, GLFW_KEY_3) == GLFW_PRESS) loadScene(3);
        if (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS) loadScene(currentScene);

        // --- Physics ---
        if (!paused) {
            int subSteps = 3;
            for (int s = 0; s < subSteps; ++s) {
                solver.update();
            }
        }

        // --- Render ---
        renderer.beginFrame();
        fluidRenderer.updateParticles(solver.getParticles());
        fluidRenderer.render(currentTime);
        renderer.endFrame();
    }

    fluidRenderer.cleanup();
    std::cout << "[Exit] Simulation ended.\n";
    return 0;
}
