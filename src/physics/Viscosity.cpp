#include "../../include/physics/Viscosity.hpp"
#include "../../include/core/SPHKernels.hpp"
#include <cmath>

namespace Viscosity {

    void computeViscosityForce(
        std::vector<Particle>& particles,
        const std::vector<std::vector<int>>& neighborLists,
        float h,
        float viscCoeff,
        float viscLapCoeff,
        float mass)
    {
        for (size_t i = 0; i < particles.size(); ++i) {
            auto& pi = particles[i];
            Vec2 fVisc(0.0f, 0.0f);

            for (int jIdx : neighborLists[i]) {
                if (static_cast<size_t>(jIdx) == i) continue;

                const auto& pj = particles[jIdx];
                Vec2 rij = pj.position - pi.position;
                float r = rij.length();

                if (r < h && r > 1e-6f) {
                    // F_visc = μ * m_j * (v_j - v_i) / ρ_j * ∇²W_visc
                    Vec2 velDiff = pj.velocity - pi.velocity;
                    float viscMag = viscCoeff * mass / pj.density *
                        SPHKernels::viscLaplacian(r, h, viscLapCoeff);
                    fVisc += velDiff * viscMag;
                }
            }

            pi.force += fVisc;
        }
    }

    void applyXSPH(
        std::vector<Particle>& particles,
        const std::vector<std::vector<int>>& neighborLists,
        float h,
        float epsilon,
        float poly6Coeff,
        float mass)
    {
        float hSq = h * h;
        std::vector<Vec2> corrections(particles.size(), Vec2(0.0f, 0.0f));

        for (size_t i = 0; i < particles.size(); ++i) {
            const auto& pi = particles[i];

            for (int jIdx : neighborLists[i]) {
                if (static_cast<size_t>(jIdx) == i) continue;

                const auto& pj = particles[jIdx];
                Vec2 rij = pj.position - pi.position;
                float r2 = rij.lengthSq();

                if (r2 < hSq) {
                    float avgDensity = (pi.density + pj.density) * 0.5f;
                    float w = SPHKernels::poly6(r2, h, poly6Coeff);
                    corrections[i] += (pj.velocity - pi.velocity) * (mass / avgDensity * w);
                }
            }
        }

        for (size_t i = 0; i < particles.size(); ++i) {
            particles[i].velocity += corrections[i] * epsilon;
        }
    }

} // namespace Viscosity
