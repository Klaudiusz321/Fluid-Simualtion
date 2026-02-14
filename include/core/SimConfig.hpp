#pragma once

/// Simulation configuration constants
namespace SimConfig {
    // Window
    constexpr float WINDOW_WIDTH  = 1280.0f;
    constexpr float WINDOW_HEIGHT = 720.0f;

    // SPH parameters
    constexpr float H           = 16.0f;        // Smoothing radius
    constexpr float HSQ         = H * H;         // H squared (precomputed)
    constexpr float PARTICLE_MASS = 2.5f;        // Particle mass
    constexpr float REST_DENSITY  = 300.0f;      // Rest density (ρ₀)
    constexpr float GAS_CONSTANT  = 2000.0f;     // Stiffness for Tait EOS
    constexpr float VISCOSITY     = 200.0f;      // Viscosity coefficient (μ)
    constexpr float DT            = 0.0007f;     // Fixed timestep
    constexpr float GRAVITY       = 9810.0f;     // Gravity (mm/s² ≈ 9.81 m/s² * 1000)

    // Surface tension coefficient (σ)
    constexpr float SURFACE_TENSION = 0.0728f;
    
    // XSPH velocity smoothing factor (ε)
    constexpr float XSPH_EPSILON  = 0.5f;

    // Boundary damping (energy loss on collision)
    constexpr float BOUNDARY_DAMPING = -0.5f;

    // CFL condition safety factor
    constexpr float CFL_FACTOR    = 0.4f;

    // Particle spacing for initialization
    constexpr float PARTICLE_SPACING = H * 0.5f;
}
