#pragma once

#include "../math/Vec2.hpp"
#include "../core/Particle.hpp"
#include <vector>

/// Viscosity models for SPH simulation.
///
/// Standard SPH artificial viscosity:
///   F_visc = μ Σ_j m_j (v_j - v_i) / ρ_j * ∇²W
///
/// XSPH velocity correction (Monaghan 1989):
///   v̄_i = v_i + ε Σ_j m_j / ρ̄_ij * (v_j - v_i) * W_ij
///   where ρ̄_ij = (ρ_i + ρ_j) / 2
///
/// This smooths the velocity field, reducing noise and making
/// the fluid look much more coherent.
namespace Viscosity {

    /// Compute standard SPH viscosity force.
    /// Results are accumulated into particle.force.
    void computeViscosityForce(
        std::vector<Particle>& particles,
        const std::vector<std::vector<int>>& neighborLists,
        float h,
        float viscCoeff,
        float viscLapCoeff,
        float mass
    );

    /// Apply XSPH velocity correction after integration.
    /// Modifies particle.velocity directly.
    void applyXSPH(
        std::vector<Particle>& particles,
        const std::vector<std::vector<int>>& neighborLists,
        float h,
        float epsilon,
        float poly6Coeff,
        float mass
    );

} // namespace Viscosity
