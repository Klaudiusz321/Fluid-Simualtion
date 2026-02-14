#include "../../include/physics/VorticityConfinement.hpp"
#include "../../include/core/SPHKernels.hpp"
#include <cmath>

namespace VorticityConfinement {

    std::vector<float> computeVorticity(
        const std::vector<Particle>& particles,
        const std::vector<std::vector<int>>& neighborLists,
        float h,
        float spikyGradCoeff,
        float mass)
    {
        std::vector<float> omega(particles.size(), 0.0f);

        for (size_t i = 0; i < particles.size(); ++i) {
            const auto& pi = particles[i];
            float w = 0.0f;

            for (int jIdx : neighborLists[i]) {
                if (static_cast<size_t>(jIdx) == i) continue;

                const auto& pj = particles[jIdx];
                Vec2 rij = pj.position - pi.position;
                float r = rij.length();

                if (r < h && r > 1e-6f) {
                    Vec2 rNorm = rij * (1.0f / r);
                    float gradW = SPHKernels::spikyGrad(r, h, spikyGradCoeff);

                    // ∇W vector
                    Vec2 gradWVec = rNorm * gradW;

                    // velocity difference
                    Vec2 vDiff = pj.velocity - pi.velocity;

                    // 2D cross product: (v_j - v_i) × ∇W_ij (scalar)
                    // ω_i += m_j/ρ_j * cross(vDiff, gradW)
                    w += (mass / pj.density) * vDiff.cross(gradWVec);
                }
            }

            omega[i] = w;
        }

        return omega;
    }

    void applyVorticityConfinement(
        std::vector<Particle>& particles,
        const std::vector<float>& vorticity,
        const std::vector<std::vector<int>>& neighborLists,
        float h,
        float spikyGradCoeff,
        float mass,
        float epsilon)
    {
        // Compute gradient of |ω|
        for (size_t i = 0; i < particles.size(); ++i) {
            auto& pi = particles[i];
            Vec2 gradOmegaMag(0.0f, 0.0f);

            for (int jIdx : neighborLists[i]) {
                if (static_cast<size_t>(jIdx) == i) continue;

                const auto& pj = particles[jIdx];
                Vec2 rij = pj.position - pi.position;
                float r = rij.length();

                if (r < h && r > 1e-6f) {
                    Vec2 rNorm = rij * (1.0f / r);
                    float gradW = SPHKernels::spikyGrad(r, h, spikyGradCoeff);

                    // N = ∇|ω| = Σ m_j/ρ_j * |ω_j| * ∇W
                    gradOmegaMag += rNorm * (mass / pj.density * std::abs(vorticity[jIdx]) * gradW);
                }
            }

            float gradLen = gradOmegaMag.length();
            if (gradLen > 1e-6f) {
                Vec2 N = gradOmegaMag / gradLen;  // Normalized

                // In 2D: F_vort = ε * (N̂_perp * ω)
                // N̂ × ω (where ω is in z) gives a force in the xy-plane
                Vec2 fVort = N.perp() * (epsilon * vorticity[i]);
                pi.force += fVort;
            }
        }
    }

} // namespace VorticityConfinement
