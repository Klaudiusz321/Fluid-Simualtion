#pragma once

#include <cmath>

/// SPH smoothing kernels and their derivatives.
/// All kernels are defined for 2D.
/// q = r / h, where r is the distance and h is the smoothing length.
namespace SPHKernels {

    constexpr float PI = 3.14159265358979323846f;

    // ============================================================
    // Poly6 Kernel — used for density estimation (2D)
    // W(r, h) = (4 / πh⁸) * (h² - r²)³  for 0 ≤ r ≤ h
    // ============================================================
    inline float poly6Coeff(float h) {
        return 4.0f / (PI * std::pow(h, 8.0f));
    }

    inline float poly6(float r2, float h, float coeff) {
        float h2 = h * h;
        if (r2 >= h2) return 0.0f;
        float diff = h2 - r2;
        return coeff * diff * diff * diff;
    }

    // Gradient of Poly6 (returns scalar multiplier, multiply by rij vector)
    inline float poly6Grad(float r2, float h, float coeff) {
        float h2 = h * h;
        if (r2 >= h2) return 0.0f;
        float diff = h2 - r2;
        return -6.0f * coeff * diff * diff;
    }

    // Laplacian of Poly6
    inline float poly6Laplacian(float r2, float h, float coeff) {
        float h2 = h * h;
        if (r2 >= h2) return 0.0f;
        float diff = h2 - r2;
        return coeff * (-12.0f * diff * diff + 24.0f * r2 * diff);
        // Simplified: coeff * diff * (-12*diff + 24*r2) but expanded for clarity
    }

    // ============================================================
    // Spiky Kernel Gradient — used for pressure force (2D)
    // grad W(r, h) = -(30 / πh⁵) * (h - r)² * (r_hat)
    // ============================================================
    inline float spikyGradCoeff(float h) {
        return -30.0f / (PI * std::pow(h, 5.0f));
    }

    /// Returns scalar multiplier for gradient (multiply by normalized rij)
    inline float spikyGrad(float r, float h, float coeff) {
        if (r >= h || r < 1e-6f) return 0.0f;
        float diff = h - r;
        return coeff * diff * diff;
    }

    // ============================================================
    // Viscosity Kernel Laplacian — used for viscosity force (2D)
    // ∇²W(r, h) = (40 / πh⁵) * (h - r)
    // ============================================================
    inline float viscLapCoeff(float h) {
        return 40.0f / (PI * std::pow(h, 5.0f));
    }

    inline float viscLaplacian(float r, float h, float coeff) {
        if (r >= h) return 0.0f;
        return coeff * (h - r);
    }

    // ============================================================
    // Wendland C2 Kernel — more stable alternative to Poly6
    // W(q) = (7 / 4πh²) * (1 - q/2)⁴ * (1 + 2q)  for q = r/h ≤ 2
    // ============================================================
    inline float wendlandC2Coeff(float h) {
        return 7.0f / (4.0f * PI * h * h);
    }

    inline float wendlandC2(float r, float h, float coeff) {
        float q = r / h;
        if (q >= 2.0f) return 0.0f;
        float t = 1.0f - 0.5f * q;
        return coeff * t * t * t * t * (1.0f + 2.0f * q);
    }

    /// Gradient magnitude of Wendland C2 (multiply by rij/r to get vector)
    inline float wendlandC2Grad(float r, float h, float coeff) {
        float q = r / h;
        if (q >= 2.0f || r < 1e-6f) return 0.0f;
        float t = 1.0f - 0.5f * q;
        // dW/dr = coeff * (-5q) * (1 - q/2)^3 / h
        return coeff * (-5.0f * q) * t * t * t / h;
    }

    // ============================================================
    // Cubic Spline Kernel — classic SPH kernel
    // ============================================================
    inline float cubicSplineCoeff(float h) {
        return 10.0f / (7.0f * PI * h * h);
    }

    inline float cubicSpline(float r, float h, float coeff) {
        float q = r / h;
        if (q >= 2.0f) return 0.0f;
        if (q <= 1.0f) {
            return coeff * (1.0f - 1.5f * q * q + 0.75f * q * q * q);
        }
        float t = 2.0f - q;
        return coeff * 0.25f * t * t * t;
    }

} // namespace SPHKernels
