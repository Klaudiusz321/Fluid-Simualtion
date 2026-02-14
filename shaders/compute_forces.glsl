#version 430 core

// =============================================================
// SPH Compute Shader — GPU-accelerated force computation
// =============================================================
// Computes pressure + viscosity + gravity forces per particle.
// Run AFTER compute_density.glsl.
// =============================================================

layout(local_size_x = 256) in;

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
uniform float uH;
uniform float uMass;
uniform float uViscosity;
uniform float uGravity;
uniform float uSpikyGradCoeff;
uniform float uViscLapCoeff;

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= uNumParticles) return;

    vec2 posI = positions[i];
    vec2 velI = velocities[i];
    float densI = densityPressure[i].x;
    float pressI = densityPressure[i].y;

    vec2 fPressure = vec2(0.0);
    vec2 fViscosity = vec2(0.0);

    for (int j = 0; j < uNumParticles; ++j) {
        if (j == int(i)) continue;

        vec2 rij = positions[j] - posI;
        float r = length(rij);

        if (r < uH && r > 1e-6) {
            vec2 rNorm = rij / r;
            float densJ = densityPressure[j].x;
            float pressJ = densityPressure[j].y;

            // Pressure force (Spiky kernel gradient)
            float diff = uH - r;
            float pressureMag = -uMass * (pressI + pressJ) / (2.0 * densJ) * uSpikyGradCoeff * diff * diff;
            fPressure += rNorm * pressureMag;

            // Viscosity force
            vec2 velDiff = velocities[j] - velI;
            float viscMag = uViscosity * uMass / densJ * uViscLapCoeff * (uH - r);
            fViscosity += velDiff * viscMag;
        }
    }

    // Gravity
    vec2 fGravity = vec2(0.0, uGravity * densI);

    forces[i] = fPressure + fViscosity + fGravity;
}
