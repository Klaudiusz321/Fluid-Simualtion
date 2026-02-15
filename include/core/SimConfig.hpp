#pragma once

/// Simulation configuration constants
/// 
/// IMPORTANT: All kernels use **2D** normalization.
/// Parameters are tuned so that at rest (square grid, spacing ≈ H/2),
/// the computed SPH density ≈ REST_DENSITY.
///
/// With 2D Poly6, h=16, spacing=8, mass=65:
///   Σ m W(r) ≈ 65 × 0.0158 ≈ 1.03  →  REST_DENSITY = 1.0
namespace SimConfig {
    // Window
    constexpr float WINDOW_WIDTH  = 1280.0f;
    constexpr float WINDOW_HEIGHT = 720.0f;

    // SPH parameters
    constexpr float H             = 16.0f;       // Smoothing radius (pixels)
    constexpr float HSQ           = H * H;        // H squared
    constexpr float PARTICLE_MASS = 65.0f;        // Tuned so density ≈ REST_DENSITY at rest
    constexpr float REST_DENSITY  = 1.0f;         // Target rest density (normalized)
    constexpr float GAS_CONSTANT  = 10000.0f;     // Stiffness (higher = more incompressible)
    constexpr float VISCOSITY     = 5.0f;         // Viscosity coefficient
    constexpr float DT            = 0.003f;       // Fixed timestep
    constexpr float GRAVITY       = 200.0f;       // Gravity (px/s²)

    // Surface tension coefficient (σ) — disabled, set > 0 to enable
    constexpr float SURFACE_TENSION = 0.0f;

    // XSPH velocity smoothing factor (ε)
    constexpr float XSPH_EPSILON  = 0.5f;

    // Boundary damping (energy loss on collision, negative = reflect)
    constexpr float BOUNDARY_DAMPING = -0.5f;

    // CFL condition safety factor
    constexpr float CFL_FACTOR    = 0.4f;

    // Particle spacing for initialization (~H/2 for good kernel overlap)
    constexpr float PARTICLE_SPACING = H * 0.5f;
}
