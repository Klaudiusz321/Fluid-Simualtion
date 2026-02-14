#version 430 core

// =============================================================
// SPH Compute Shader — GPU-accelerated density & pressure
// =============================================================
// This compute shader performs the density/pressure computation
// on the GPU, enabling 100k+ particles in real-time.
//
// Dispatch: glDispatchCompute(ceil(numParticles / 256), 1, 1)
// =============================================================

layout(local_size_x = 256) in;

// Particle data (SoA layout for GPU cache efficiency)
layout(std430, binding = 0) buffer Positions {
    vec2 positions[];
};

layout(std430, binding = 1) buffer Velocities {
    vec2 velocities[];
};

layout(std430, binding = 2) buffer Forces {
    vec2 forces[];
};

layout(std430, binding = 3) buffer DensityPressure {
    vec2 densityPressure[];  // .x = density, .y = pressure
};

uniform int   uNumParticles;
uniform float uH;           // Smoothing radius
uniform float uHSQ;         // H * H
uniform float uMass;
uniform float uRestDensity;
uniform float uGasConstant;
uniform float uViscosity;
uniform float uGravity;
uniform float uDT;

// Kernel coefficients
uniform float uPoly6Coeff;
uniform float uSpikyGradCoeff;
uniform float uViscLapCoeff;

// ============================================================
// Pass 1: Compute Density & Pressure
// ============================================================
void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= uNumParticles) return;

    vec2 posI = positions[i];
    float density = 0.0;

    // Sum contributions from all neighbors
    // NOTE: For production, use spatial hashing on GPU (separate pass)
    for (int j = 0; j < uNumParticles; ++j) {
        vec2 rij = positions[j] - posI;
        float r2 = dot(rij, rij);

        if (r2 < uHSQ) {
            float diff = uHSQ - r2;
            density += uMass * uPoly6Coeff * diff * diff * diff;
        }
    }

    // Clamp minimum density
    density = max(density, uRestDensity * 0.5);

    // Tait equation of state
    float pressure = uGasConstant * (density - uRestDensity);

    densityPressure[i] = vec2(density, pressure);
}
