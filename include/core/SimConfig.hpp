#pragma once

/// Simulation configuration constants
namespace SimConfig {
    // Window
    constexpr float WINDOW_WIDTH  = 1280.0f;
    constexpr float WINDOW_HEIGHT = 720.0f;

    // SPH parameters
    constexpr float H           = 12.0f;         // Smoothing radius (smaller = more detail)
    constexpr float HSQ         = H * H;          // H squared (precomputed)
    constexpr float PARTICLE_MASS = 1.0f;         // Particle mass
    constexpr float REST_DENSITY  = 1000.0f;      // Rest density (ρ₀) — water-like
    constexpr float GAS_CONSTANT  = 2000.0f;      // Stiffness for Tait EOS (higher = snappier)
    constexpr float VISCOSITY     = 60.0f;        // Viscosity coefficient (μ) — low = fluid flows freely
    constexpr float DT            = 0.0005f;      // Fixed timestep
    constexpr float GRAVITY       = 0.0f;         // No gravity (top-down view)

    // Mouse interaction
    constexpr float MOUSE_FORCE_RADIUS = 80.0f;   // Radius of mouse influence
    constexpr float MOUSE_FORCE_STRENGTH = 150000.0f; // Strength of mouse push

    // Surface tension coefficient (σ)
    constexpr float SURFACE_TENSION = 0.3f;

    // XSPH velocity smoothing factor (ε)
    constexpr float XSPH_EPSILON  = 0.1f;

    // Boundary damping (energy loss on collision)
    constexpr float BOUNDARY_DAMPING = -0.5f;

    // CFL condition safety factor
    constexpr float CFL_FACTOR    = 0.4f;

    // Particle spacing for initialization
    constexpr float PARTICLE_SPACING = H * 0.55f;
}
