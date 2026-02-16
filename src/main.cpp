#include "../include/core/SPHSolver.hpp"
#include "../include/render/Renderer.hpp"
#include "../include/render/FluidRenderer.hpp"
#include "../include/core/SimConfig.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <cmath>

// ============================================================
// SPH Fluid Simulation — Top-Down View
// ============================================================
//
// Controls:
//   1/2/3   — Switch scene (dam / pool / tall-dam)
//   R       — Reset current scene
//   SPACE   — Pause / Resume
//   Mouse   — Drag (LMB) to push fluid
//   ESC     — Quit
// ============================================================

// Mouse state (for drag interaction)
static double gMouseX = 0.0, gMouseY = 0.0;
static double gPrevMouseX = 0.0, gPrevMouseY = 0.0;
static bool   gMouseDragging = false;

static void cursorCallback(GLFWwindow* /*win*/, double xpos, double ypos) {
    gPrevMouseX = gMouseX;
    gPrevMouseY = gMouseY;
    gMouseX = xpos;
    gMouseY = ypos;
}

static void mouseButtonCallback(GLFWwindow* /*win*/, int button, int action, int /*mods*/) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        gMouseDragging = (action == GLFW_PRESS);
        if (gMouseDragging) {
            // Reset prev so we don't get a huge delta on first press
            gPrevMouseX = gMouseX;
            gPrevMouseY = gMouseY;
        }
    }
}

int main() {
    // --- Init Window ---
    Renderer renderer;
    int width  = static_cast<int>(SimConfig::WINDOW_WIDTH);
    int height = static_cast<int>(SimConfig::WINDOW_HEIGHT);

    if (!renderer.init(width, height, "SPH Fluid — Top Down")) {
        return -1;
    }

    // --- Register mouse callbacks ---
    GLFWwindow* win = renderer.getWindow();
    glfwSetCursorPosCallback(win, cursorCallback);
    glfwSetMouseButtonCallback(win, mouseButtonCallback);

    // --- Init Fluid Renderer ---
    FluidRenderer fluidRenderer;
    if (!fluidRenderer.init(width, height)) {
        return -1;
    }
    fluidRenderer.setFluidColor(0.1f, 0.4f, 0.9f, 0.9f);

    // --- Init Solver ---
    SPHSolver solver;

    int currentScene = 2;  // default: pool (top-down view)
    auto loadScene = [&](int scene) {
        currentScene = scene;
        switch (scene) {
            case 1: solver.initDamBreak(40, 20); break;
            case 2: solver.initPool(0.85f);      break;   // fills 85% — nearly full
            case 3: solver.initTallDam();         break;
            default: solver.initPool(0.85f);      break;
        }
        std::cout << "[Scene " << scene << "] " << solver.numParticles() << " particles\n";
    };
    loadScene(2);

    // --- State ---
    bool paused = false;
    float lastTime = renderer.getTime();
    int frameCount = 0;
    float fpsTimer = 0.0f;

    // Point-size enable (required for gl_PointSize in vertex shader)
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    std::cout << "\n=== SPH Fluid — Top-Down View ===\n";
    std::cout << "Controls:\n";
    std::cout << "  Mouse LMB drag — push fluid\n";
    std::cout << "  1/2/3  — Switch scene\n";
    std::cout << "  R      — Reset\n";
    std::cout << "  SPACE  — Pause / Resume\n";
    std::cout << "  ESC    — Quit\n\n";

    // --- Main Loop ---
    while (!renderer.shouldClose()) {
        renderer.pollEvents();

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

        // --- Mouse drag → push fluid ---
        if (gMouseDragging && !paused) {
            float dx = static_cast<float>(gMouseX - gPrevMouseX);
            float dy = static_cast<float>(gMouseY - gPrevMouseY);
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 0.5f) {
                solver.applyForceAt(
                    static_cast<float>(gMouseX),
                    static_cast<float>(gMouseY),
                    dx, dy,
                    SimConfig::MOUSE_FORCE_RADIUS,
                    SimConfig::MOUSE_FORCE_STRENGTH
                );
            }
        }

        // --- Physics ---
        if (!paused) {
            int subSteps = 4;
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
