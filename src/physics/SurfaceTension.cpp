#include "../../include/physics/SurfaceTension.hpp"
#include "../../include/core/SPHKernels.hpp"
#include <cmath>

namespace SurfaceTension {

    void computeColorField(
        std::vector<Particle>& particles,
        const std::vector<std::vector<int>>& neighborLists,
        float h,
        float poly6Coeff)
    {
        float hSq = h * h;

        for (size_t i = 0; i < particles.size(); ++i) {
            auto& pi = particles[i];
            pi.colorField = 0.0f;
            pi.colorGradient = Vec2(0.0f, 0.0f);
            pi.colorLaplacian = 0.0f;

            for (int jIdx : neighborLists[i]) {
                const auto& pj = particles[jIdx];
                Vec2 rij = pj.position - pi.position;
                float r2 = rij.lengthSq();

                if (r2 < hSq) {
                    float massOverDensity = pj.mass / pj.density;

                    // Color field: c_i = Σ m_j/ρ_j * W
                    pi.colorField += massOverDensity * SPHKernels::poly6(r2, h, poly6Coeff);

                    // Gradient: ∇c_i = Σ m_j/ρ_j * ∇W
                    float gradMag = SPHKernels::poly6Grad(r2, h, poly6Coeff);
                    pi.colorGradient += rij * (massOverDensity * gradMag);

                    // Laplacian: ∇²c_i = Σ m_j/ρ_j * ∇²W
                    pi.colorLaplacian += massOverDensity * SPHKernels::poly6Laplacian(r2, h, poly6Coeff);
                }
            }
        }
    }

    void applySurfaceTensionForce(
        std::vector<Particle>& particles,
        float sigma,
        float threshold)
    {
        for (auto& pi : particles) {
            float gradLen = pi.colorGradient.length();
            if (gradLen > threshold) {
                // Curvature: κ = -∇²c / |∇c|
                float curvature = -pi.colorLaplacian / gradLen;

                // Surface tension force: F = σ * κ * n̂ * ρ
                Vec2 normal = pi.colorGradient.normalized();
                pi.force += normal * (sigma * curvature * pi.density);
            }
        }
    }

} // namespace SurfaceTension
