#pragma once

#include "../math/Vec2.hpp"
#include "../core/Particle.hpp"
#include <vector>

/// Vorticity confinement for SPH.
///
/// Numerical dissipation in SPH kills small-scale vortices.
/// Vorticity confinement re-injects energy into the velocity field
/// at locations where vorticity is present.
///
/// In 2D, vorticity ω is a scalar (the z-component of ∇ × v):
///   ω_i = Σ_j m_j / ρ_j * (v_j - v_i) × ∇W_ij   (scalar cross product)
///
/// The corrective force:
///   F_vort = ε (N̂ × ω)
///   where N = ∇|ω|, N̂ = N / |N|
namespace VorticityConfinement {

    /// Compute vorticity (scalar in 2D) for each particle.
    /// Returns vector of ω_i values.
    std::vector<float> computeVorticity(
        const std::vector<Particle>& particles,
        const std::vector<std::vector<int>>& neighborLists,
        float h,
        float spikyGradCoeff,
        float mass
    );

    /// Apply vorticity confinement force.
    /// Accumulates into particle.force.
    /// epsilon controls the strength of the confinement.
    void applyVorticityConfinement(
        std::vector<Particle>& particles,
        const std::vector<float>& vorticity,
        const std::vector<std::vector<int>>& neighborLists,
        float h,
        float spikyGradCoeff,
        float mass,
        float epsilon = 0.01f
    );

} // namespace VorticityConfinement
