#pragma once

#include "../math/Vec2.hpp"
#include "../core/Particle.hpp"
#include "../core/SPHKernels.hpp"
#include <vector>

/// Continuum Surface Force (CSF) model for surface tension.
/// Based on: Brackbill, Kothe, Zemach (1992)
///
/// The idea:
/// 1. Compute color field: c_i = Σ_j (m_j / ρ_j) W_ij
/// 2. Compute gradient of color field (surface normal): n_i = ∇c_i
/// 3. Compute Laplacian of color field (curvature): κ = -∇²c / |n|
/// 4. Surface tension force: F_st = -σ κ n̂
///
/// Only apply force where |n| exceeds a threshold (near the surface).
namespace SurfaceTension {

    /// Compute the color field, its gradient, and Laplacian for all particles.
    /// These are stored in particle.colorField, particle.colorGradient, particle.colorLaplacian.
    void computeColorField(
        std::vector<Particle>& particles,
        const std::vector<std::vector<int>>& neighborLists,
        float h,
        float poly6Coeff
    );

    /// Compute and accumulate surface tension force into particle.force
    /// σ = surface tension coefficient
    /// threshold = minimum |∇c| to be considered a surface particle
    void applySurfaceTensionForce(
        std::vector<Particle>& particles,
        float sigma,
        float threshold = 6.0f
    );

} // namespace SurfaceTension
