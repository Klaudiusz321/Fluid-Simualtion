#pragma once

#include <vector>
#include "Particle.hpp"
#include "SpatialHash.hpp"
#include "SPHKernels.hpp"
#include "SimConfig.hpp"

/// Main SPH fluid solver using Weakly Compressible SPH (WCSPH)
/// with spatial hashing, XSPH correction, and surface tension.
class SPHSolver {
public:
    SPHSolver();

    /// Initialize a "dam break" block of particles
    void initDamBreak(int rows, int cols);

    /// Initialize particles in a circular droplet
    void initDroplet(float cx, float cy, float radius);

    /// Initialize a pool of water filling the bottom portion of the screen
    void initPool(float fillFraction = 0.4f);

    /// Initialize dam break with a tall column on the left
    void initTallDam();

    /// Main physics step
    void update();

    /// Adaptive timestep based on CFL condition
    float computeAdaptiveDT() const;

    /// Access particles for rendering
    const std::vector<Particle>& getParticles() const { return particles_; }
    std::vector<Particle>& getParticles() { return particles_; }

    /// Number of particles
    size_t numParticles() const { return particles_.size(); }

    /// Add a single particle at runtime (e.g., for emitters)
    void addParticle(float x, float y);

private:
    std::vector<Particle> particles_;
    std::vector<Vec2> positions_;          // contiguous positions for spatial hash
    SpatialHash spatialHash_;
    
    // Precomputed kernel coefficients
    float poly6Coeff_;
    float spikyGradCoeff_;
    float viscLapCoeff_;
    float wendlandCoeff_;

    int nextId_;

    // --- Simulation pipeline ---
    void buildSpatialHash();
    void computeDensityPressure();
    void computeForces();
    void computeSurfaceTension();
    void applyXSPH();
    void integrate();
    void enforceBoundaries();
};
