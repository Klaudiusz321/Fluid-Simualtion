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
    constexpr float GAS_CONSTANT  = 1500.0f;      // Stiffness for Tait EOS
    constexpr float VISCOSITY     = 250.0f;       // Viscosity coefficient (μ)
    constexpr float DT            = 0.0005f;      // Fixed timestep
    constexpr float GRAVITY       = 12000.0f;     // Gravity (tuned for visual appeal)

    // Surface tension coefficient (σ)
    constexpr float SURFACE_TENSION = 0.5f;

    // XSPH velocity smoothing factor (ε)
    constexpr float XSPH_EPSILON  = 0.3f;

    // Boundary damping (energy loss on collision)
    constexpr float BOUNDARY_DAMPING = -0.3f;

    // CFL condition safety factor
    constexpr float CFL_FACTOR    = 0.4f;

    // Particle spacing for initialization
    constexpr float PARTICLE_SPACING = H * 0.55f;
}
